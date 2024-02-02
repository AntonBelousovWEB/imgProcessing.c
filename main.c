#include <stdio.h>
#include <assert.h>
#include <windows.h>

#define STB_IMAGE_IMPLEMENTATION
#include "./Headers/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "./Headers/stb_image_write.h"

#define NN_IMPLEMENTATION
#include "./Headers/nn.h"

char *args_shift(int *argc, char ***argv) {
    assert(*argc > 0);
    char *result = **argv;
    (*argc) -= 1;
    (*argv) += 1;
    return result;
}

char *matrix_to_string(Mat *matrix) {
    size_t buffer_size = 0;
    for (size_t i = 0; i < matrix->rows; ++i) {
        for (size_t j = 0; j < matrix->cols; ++j) {
            buffer_size += snprintf(NULL, 0, "%f ", MAT_AT(*matrix, i, j));
        }
        buffer_size += snprintf(NULL, 0, "\n");
    }
    char *buffer = malloc(buffer_size + 1);
    if (buffer == NULL) {
        return NULL;
    }
    char *ptr = buffer;
    for (size_t i = 0; i < matrix->rows; ++i) {
        for (size_t j = 0; j < matrix->cols; ++j) {
            ptr += sprintf(ptr, "%f ", MAT_AT(*matrix, i, j));
        }
        ptr += sprintf(ptr, "\n");
    }
    return buffer;
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

    for(int y = 0; y < img_height; ++y) {
        for(int x = 0; x < img_width; ++x) {
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

    const char *out_file_path = "img.txt";
    FILE *out = fopen(out_file_path, "w");
    FILE *out_ = fopen(out_file_path, "r");
    if(out == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", out_file_path);
    }

    size_t arch[] = {2, 7, 1};
    NN nn = nn_alloc(arch, ARRAY_LEN(arch));
    NN g = nn_alloc(arch, ARRAY_LEN(arch));
    nn_rand(nn, -1, 1);
    
    float rate = 1.0f;
    size_t max_epochs = 1000000;
    float const best = nn_cost(nn, ti, to)*100000000;
    for(size_t epoch = 0; epoch < max_epochs; ++epoch) {
        nn_backprop(nn, g, ti, to);
        nn_learn(nn, g, rate);

        if(epoch % 10000 == 0) {
            system("cls");
            printf("%d / %d . Best value for maximum epochs %f\n\n", epoch, max_epochs, best);
            
            char *matrix_string = matrix_to_string(&t);
            fwrite(matrix_string, strlen(matrix_string), 1, out);
            fclose(out);

            // for(size_t y = 0; y < (size_t) img_height; ++y) {
            //     for(size_t x = 0; x < (size_t) img_width; ++x) {
            //         // MAT_AT(NN_INPUT(nn), 0, 0) = (float)x/(img_width - 1);
            //         // MAT_AT(NN_INPUT(nn), 0, 1) = (float)y/(img_height - 1);
            //         // nn_forward(nn);
            //         // uint8_t pixel = MAT_AT(NN_OUTPUT(nn), 0, 0)*255.f;
            //         // if(pixel) printf("%3u ", pixel); else printf("    ");
            //     }
            //     printf("\n");
            // }
            // Sleep(100); // if you want :-)
        }
    }

    size_t out_width = 512;
    size_t out_height = 512; 
    uint8_t *out_pixels = malloc(sizeof(*out_pixels)*out_width*out_height);
    assert(out_pixels != NULL);

    for(size_t y = 0; y < out_height; ++y) {
        for(size_t x = 0; x < out_width; ++x) {
            MAT_AT(NN_INPUT(nn), 0, 0) = (float)x/(out_width - 1);
            MAT_AT(NN_INPUT(nn), 0, 1) = (float)y/(out_height - 1);
            nn_forward(nn);
            uint8_t pixel = MAT_AT(NN_OUTPUT(nn), 0, 0)*255.f;
            out_pixels[y*out_width + x] = pixel;
        }
    }

    const char *out_file_path_img = "./Images/generated/img.png";
    if(!stbi_write_png(out_file_path_img, out_width, out_height, 1, out_pixels, out_width*sizeof(*out_pixels))) {
        fprintf(stderr, "Error: could not save image %s\n", out_file_path_img);
        return 1;
    }

    printf("Generated %s from %s\n", out_file_path_img, img_file_path);

    printf("Real: \n");
    for(size_t y = 0; y < (size_t) img_height; ++y) {
        for(size_t x = 0; x < (size_t) img_width; ++x) {
            uint8_t pixel = img_pixels[y*img_width + x];
            if(pixel) printf("%3u ", pixel); else printf("    ");
        }
        printf("\n");
    }

    return 0;
}