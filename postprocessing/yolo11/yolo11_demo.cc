// YOLO11 example: run the model on the NPU with qbruntime, decode and
// postprocess the raw outputs on the SP500 DSP, draw the boxes, save a PNG.
// The input image must already be 640x640, the model input size.
//
//   ./yolo11_demo <model.mxq> <640x640 image> [output png]

#include <qbruntime/qbruntime.h>
#include <sp500/yolo.h>

#include <cstdio>
#include <memory>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

// COCO class names. The model output class_id is an index into this table.
static const char* kClassNames[80] = {
    "person",        "bicycle",      "car",
    "motorcycle",    "airplane",     "bus",
    "train",         "truck",        "boat",
    "traffic light", "fire hydrant", "stop sign",
    "parking meter", "bench",        "bird",
    "cat",           "dog",          "horse",
    "sheep",         "cow",          "elephant",
    "bear",          "zebra",        "giraffe",
    "backpack",      "umbrella",     "handbag",
    "tie",           "suitcase",     "frisbee",
    "skis",          "snowboard",    "sports ball",
    "kite",          "baseball bat", "baseball glove",
    "skateboard",    "surfboard",    "tennis racket",
    "bottle",        "wine glass",   "cup",
    "fork",          "knife",        "spoon",
    "bowl",          "banana",       "apple",
    "sandwich",      "orange",       "broccoli",
    "carrot",        "hot dog",      "pizza",
    "donut",         "cake",         "chair",
    "couch",         "potted plant", "bed",
    "dining table",  "toilet",       "tv",
    "laptop",        "mouse",        "remote",
    "keyboard",      "cell phone",   "microwave",
    "oven",          "toaster",      "sink",
    "refrigerator",  "book",         "clock",
    "vase",          "scissors",     "teddy bear",
    "hair drier",    "toothbrush",
};

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: %s <model.mxq> <640x640 image> [output png]\n",
                     argv[0]);
        return 1;
    }
    const char* model_path = argv[1];
    const char* input_path = argv[2];
    const char* output_path = argc > 3 ? argv[3] : "yolo11_out.png";

    // 1. Read the image as 8-bit BGR HWC. It must match the model input size.
    const int input_w = 640;
    const int input_h = 640;
    cv::Mat image = cv::imread(input_path, cv::IMREAD_COLOR);
    if (image.empty()) {
        std::fprintf(stderr, "cannot read %s\n", input_path);
        return 1;
    }
    if (image.cols != input_w || image.rows != input_h) {
        std::fprintf(stderr, "image must be %dx%d, got %dx%d\n", input_w, input_h,
                     image.cols, image.rows);
        return 1;
    }

    // 2. Connect to the DSP. Fails with SP500_ERR_TIMEOUT if no firmware is loaded.
    sp500_status rc = sp500_dsp_open(0);
    if (rc != SP500_OK) {
        std::fprintf(stderr, "sp500_dsp_open: %s\n", sp500_status_string(rc));
        return 1;
    }

    // 3. Open the NPU and load the model.
    mobilint::StatusCode sc;
    std::unique_ptr<mobilint::Accelerator> npu = mobilint::Accelerator::create(0, sc);
    if (!sc || npu == nullptr) {
        std::fprintf(stderr, "cannot open NPU\n");
        return 1;
    }
    std::unique_ptr<mobilint::Model> model = mobilint::Model::create(model_path, sc);
    if (!sc || model == nullptr) {
        std::fprintf(stderr, "cannot load %s\n", model_path);
        return 1;
    }
    sc = model->launch(*npu);
    if (!sc) {
        std::fprintf(stderr, "cannot launch model\n");
        return 1;
    }

    // 4. Convert BGR to RGB. The model expects RGB HWC uint8.
    cv::Mat rgb;
    cv::cvtColor(image, rgb, cv::COLOR_BGR2RGB);

    // 5. Run the NPU. YOLO11 gives six float outputs, one box and one class
    //    plane per level: [box 80x80x64, cls 80x80x80, box 40x40x64, cls 40x40x80,
    //    box 20x20x64, cls 20x20x80].
    std::vector<uint8_t*> inputs = {rgb.data};
    std::vector<std::vector<float>> outputs = model->infer(inputs, sc);
    if (!sc || outputs.size() != 6) {
        std::fprintf(stderr, "NPU inference failed\n");
        return 1;
    }

    // 6. Decode + postprocess on the DSP. Each level takes its box and class plane.
    sp500_yolo_raw raw;
    sp500_yolo_raw_init(&raw);
    for (int level = 0; level < 3; ++level) {
        std::vector<float>& box = outputs[level * 2];
        std::vector<float>& cls = outputs[level * 2 + 1];
        raw.box[level] = sp500_buffer_wrap(box.data(), box.size() * sizeof(float));
        raw.cls[level] = sp500_buffer_wrap(cls.data(), cls.size() * sizeof(float));
    }

    sp500_yolo_params yp;
    sp500_yolo_params_init(&yp);
    yp.img_w = input_w;         // model input width
    yp.img_h = input_h;         // model input height
    yp.num_classes = 80;        // COCO
    yp.max_detections = 100;    // upper bound on returned boxes
    yp.conf_threshold = 0.25f;  // drop boxes below this score
    yp.iou_threshold = 0.7f;    // NMS overlap threshold
    // anchors_* and proto/mask_* fields stay at their init values: this is an
    // anchor-free detection model.

    int32_t head;
    rc = sp500_yolo_head_for_model("yolo11", &head);
    if (rc != SP500_OK) {
        std::fprintf(stderr, "sp500_yolo_head_for_model: %s\n", sp500_status_string(rc));
        return 1;
    }

    sp500_yolo_detection dets[100];  // room for max_detections records
    sp500_yolo_output out;
    sp500_yolo_output_init(&out);
    out.records = sp500_buffer_wrap(dets, sizeof(dets));

    sp500_yolo_result yr;
    rc = sp500_yolo_decode(head, SP500_YOLO_TASK_DETECT, &raw, &yp, &out, &yr);
    if (rc != SP500_OK) {
        std::fprintf(stderr, "sp500_yolo_decode: %s\n", sp500_status_string(rc));
        return 1;
    }
    std::printf("detections %u, decode %.3f ms\n", yr.count, yr.host_seconds * 1000.0);

    // 7. Draw the boxes. Coordinates are pixels in the 640x640 input, which is
    //    the original image here.
    for (uint32_t i = 0; i < yr.count; ++i) {
        const sp500_yolo_detection& d = dets[i];
        int x1 = static_cast<int>(d.x1);
        int y1 = static_cast<int>(d.y1);
        int x2 = static_cast<int>(d.x2);
        int y2 = static_cast<int>(d.y2);
        const char* name = kClassNames[static_cast<int>(d.class_id)];

        std::printf("  %-14s %.2f  [%d, %d, %d, %d]\n", name, d.score, x1, y1, x2, y2);
        cv::rectangle(image, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(0, 255, 0),
                      2);
        char label[64];
        std::snprintf(label, sizeof(label), "%s %.2f", name, d.score);
        int label_y = y1 > 20 ? y1 - 6 : y1 + 20;  // keep the label inside the image
        cv::putText(image, label, cv::Point(x1, label_y), cv::FONT_HERSHEY_SIMPLEX, 0.6,
                    cv::Scalar(0, 255, 0), 2);
    }

    // 8. Save the result.
    if (!cv::imwrite(output_path, image)) {
        std::fprintf(stderr, "cannot write %s\n", output_path);
        return 1;
    }
    std::printf("saved %s\n", output_path);

    // 9. Release the model.
    model->dispose();
    return 0;
}
