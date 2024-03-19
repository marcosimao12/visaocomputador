#include <stdio.h>
#include "vc.c"

int main(void){
    IVC *src;
    IVC *dst;
    int i;

    src = vc_read_image("C:/Users/marcocarvalho/Desktop/aula4/cells.pgm");

    if (src == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tFile not found!\n");
        getchar();
        return 0;
    }
    dst = vc_image_new(src->width,src->height,1,src->levels);
//    vc_gray_to_binary(src,dst, 130);
//    vc_gray_to_binary_mean_threshold(src, dst);
    int kernel = 25;
    vc_gray_midpoint_threshold(src, dst, kernel);
    for (i = 0; i<dst->bytesperline*dst->height; i+=dst->channels) {
        dst->data[i] = 255 - dst->data[i];
    }
    vc_write_image("flir01_conversion.pgm",dst);
    vc_image_free(src);
    vc_image_free(dst);

    printf("Press any key to exit....\n");
    getchar();

    return 0;
}

//primeira aula
/*
int main2(void){
    IVC *image;
    int i;

    image = vc_read_image("C:/Users/diogduarte/Desktop/VPC/Images/FLIR/flir-01.pgm");
    if (image == NULL)
    {
        printf("ERROR -> vc_read_image():\n\tFile not found!\n");
        getchar();
        return 0;
    }

    for (i = 0; i<image->bytesperline*image->height; i+=image->channels) {
        image->data[i] = 255 - image->data[i];
    }

    vc_write_image("vc_0001.pgm", image);
    vc_image_free(image);

    printf("Press any key to exit....\n");
    getchar();

    return 0;

}*/
