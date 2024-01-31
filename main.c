#include <stdio.h>
#include <assert.h>

#define STB_IMAGE_IMPLEMENTATION
#include "./Headers/stb_image.h"

#define NN_IMPLEMENTATION
#include "./Headers/nn.h"

char *args_shift(int *argc, char ***argv) {
    assert(*argc > 0);
    char *result = **argv;
    (*argc) -= 1;
    (*argv) += 1;
    return result;
}

int main(int argc, char **argv) {
    const char *program = args_shift(&argc, &argv);
    if(argc <= 0) {
        fprintf(stderr, "Usage: %s <input.png>\n", program);
        fprintf(stderr, "Error: no input file is provided\n");
    }
    const char *img_file_path = args_shift(&argc, &argv);

    int img_width, img_height, img_comp;

    uint8_t *img_pixels = (uint8_t *)stbi_load(img_file_path, &img_width, &img_height, &img_comp, 0);

    if(img_pixels == NULL) {
        fprintf(stderr, "Error: could not read image %s\n", img_file_path);
        return 1;
    }

    if(img_comp != 1) {
        fprintf(stderr, 
                "Error: %s is %d bits image. Only 8 bit grayscale images are supported\n", 
                img_file_path, img_comp*8);
        return 1;
    }

    printf("%s size %dx%d %d bits\n", 
            img_file_path, img_width, 
            img_height, img_comp*8);

    Mat t = mat_alloc(img_width*img_height, 3);

    for(int y = 0; y < img_height; y++) {
        for(int x = 0; x < img_width; x++) {
            size_t i = y*img_width + x;
            MAT_AT(t, i, 0) = (float)x/(img_width - 1);
            MAT_AT(t, i, 1) = (float)y/(img_height - 1);
            MAT_AT(t, i, 2) = img_pixels[i]/255.f;
        }
        printf("\n");
    }

    Mat ti = {
        .rows = t.rows,
        .cols = 2,
        .stride = t.stride,
        .es = &MAT_AT(t, 0, 0),
    };
    Mat to = {
        .rows = t.rows,
        .cols = 1,
        .stride = t.stride,
        .es = &MAT_AT(t, 0, ti.cols),
    };

    MAT_PRINT(ti);
    MAT_PRINT(to);

    const char *out_file_path = "img.mat";
    FILE *out = fopen(out_file_path, "wb");
    if(out == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", out_file_path);
    }
    mat_save(out, t);

    size_t arch[] = {2, 28, 1};
    NN nn = nn_alloc(arch, ARRAY_LEN(arch));
    NN g = nn_alloc(arch, ARRAY_LEN(arch));
    nn_rand(nn, -1, 1);

    float rate = 1.0f;
    size_t max_epochs = 10*1000;
    for(size_t epoch = 0; epoch < max_epochs; ++epoch) {
        nn_backprop(nn, g, ti, to);
        nn_learn(nn, g, rate);
        if(epoch % 100 == 0)
            printf("%zu: cost = %f\n", epoch, nn_cost(nn, ti, to));
    }

    return 0;
}