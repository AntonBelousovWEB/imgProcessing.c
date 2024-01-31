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

    MAT_PRINT(t);

    const char *out_file_path = "img.mat";
    FILE *out = fopen(out_file_path, "wb");
    if(out == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", out_file_path);
    }
    mat_save(out, t);
    printf("Generated %s from %s\n", out_file_path, img_file_path);

    return 0;
}