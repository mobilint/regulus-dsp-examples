// SP500 letterbox example: read an image, letterbox it to 640x640 on the DSP,
// save the result as PNG.
//
//   ./letterbox_demo <input image> [output png]

#include <sp500/letterbox.h>

#include <cstdio>
#include <opencv2/imgcodecs.hpp>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <input image> [output png]\n", argv[0]);
        return 1;
    }
    const char* input_path = argv[1];
    const char* output_path = argc > 2 ? argv[2] : "letterbox_out.png";

    // 1. Read the image as 8-bit BGR HWC, the SP500 input layout.
    cv::Mat image = cv::imread(input_path, cv::IMREAD_COLOR);
    if (image.empty()) {
        std::fprintf(stderr, "cannot read %s\n", input_path);
        return 1;
    }

    // 2. Connect to the DSP. Fails with SP500_ERR_TIMEOUT if no firmware is loaded.
    sp500_status rc = sp500_dsp_open(0);
    if (rc != SP500_OK) {
        std::fprintf(stderr, "sp500_dsp_open: %s\n", sp500_status_string(rc));
        return 1;
    }

    // 3. Set the letterbox parameters. Every field is written here for clarity;
    //    values other than src_w and src_h are the defaults set by params_init.
    sp500_letterbox_params params;
    sp500_letterbox_params_init(&params);
    params.src_w = image.cols;              // source width
    params.src_h = image.rows;              // source height
    params.dst_w = 640;                     // output width
    params.dst_h = 640;                     // output height
    params.channels = 3;                    // BGR
    params.interp = SP500_RESIZE_BILINEAR;  // resize interpolation
    params.pad_value = 114;                 // border fill value
    params.center = 1;                      // split the border evenly
    params.scale_up = 1;                    // allow upscaling a smaller image
    params.swap_rb = 0;                     // keep BGR order

    // 4. Wrap the OpenCV buffer as input; allocate the output in DSP shared memory.
    sp500_buffer src = sp500_buffer_wrap(image.data, image.total() * image.elemSize());
    const uint32_t dst_bytes = params.dst_w * params.dst_h * params.channels;
    sp500_buffer dst = sp500_buffer_alloc(dst_bytes, SP500_ALIGN_NPU);
    if (dst.data == nullptr) {
        std::fprintf(stderr, "sp500_buffer_alloc failed\n");
        return 1;
    }

    // 5. Run letterbox on the DSP.
    sp500_letterbox_result result;
    rc = sp500_letterbox(&src, &params, &dst, &result);
    if (rc != SP500_OK) {
        std::fprintf(stderr, "sp500_letterbox: %s\n", sp500_status_string(rc));
        sp500_buffer_release(&dst);
        return 1;
    }
    std::printf("input %dx%d -> output %dx%d\n", params.src_w, params.src_h, params.dst_w,
                params.dst_h);
    std::printf("scale %.4f pad_x %d pad_y %d dsp %.3f ms\n", result.scale, result.pad_x,
                result.pad_y, result.host_seconds * 1000.0);

    // 6. Save the output buffer as PNG.
    cv::Mat output(params.dst_h, params.dst_w, CV_8UC3, dst.data);
    if (!cv::imwrite(output_path, output)) {
        std::fprintf(stderr, "cannot write %s\n", output_path);
        sp500_buffer_release(&dst);
        return 1;
    }
    std::printf("saved %s\n", output_path);

    // 7. Release the shared memory buffer.
    sp500_buffer_release(&dst);
    return 0;
}
