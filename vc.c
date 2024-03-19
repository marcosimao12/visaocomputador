//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de funções não seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include "vc.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// Alocar memória para uma imagem
IVC *vc_image_new(int width, int height, int channels, int levels)
{
	IVC *image = (IVC *) malloc(sizeof(IVC));

	if(image == NULL) return NULL;
	if((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char *) malloc(image->width * image->height * image->channels * sizeof(char));

	if(image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}


// Libertar memória de uma imagem
IVC *vc_image_free(IVC *image)
{
	if(image != NULL)
	{
		if(image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


char *netpbm_get_token(FILE *file, char *tok, int len)
{
	char *t;
	int c;
	
	for(;;)
	{
		while(isspace(c = getc(file)));
		if(c != '#') break;
		do c = getc(file);
		while((c != '\n') && (c != EOF));
		if(c == EOF) break;
	}
	
	t = tok;
	
	if(c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));
		
		if(c == '#') ungetc(c, file);
	}
	
	*t = 0;
	
	return tok;
}


long int unsigned_char_to_bit(unsigned char *datauchar, unsigned char *databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char *p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);
				
				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}


void bit_to_unsigned_char(unsigned char *databit, unsigned char *datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char *p = databit;

	countbits = 1;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;
				
				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}


IVC *vc_read_image(char *filename)
{
	FILE *file = NULL;
	IVC *image = NULL;
	unsigned char *tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;
	
	// Abre o ficheiro
	if((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if(strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }	// Se PBM (Binary [0,1])
		else if(strcmp(tok, "P5") == 0) channels = 1;				// Se PGM (Gray [0,MAX(level,255)])
		else if(strcmp(tok, "P6") == 0) channels = 3;				// Se PPM (RGB [0,MAX(level,255)])
		else
		{
			#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
			#endif

			fclose(file);
			return NULL;
		}
		
		if(levels == 1) // PBM
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca memória para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			if((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca memória para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			size = image->width * image->height * image->channels;

			if((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}
		
		fclose(file);
	}
	else
	{
		#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
		#endif
	}
	
	return image;
}


int vc_write_image(char *filename, IVC *image)
{
	FILE *file = NULL;
	unsigned char *tmp;
	long int totalbytes, sizeofbinarydata;
	
	if(image == NULL) return 0;

	if((file = fopen(filename, "wb")) != NULL)
	{
		if(image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;
			
			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);
			
			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if(fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
				#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
				#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);
		
			if(fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
				#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
				#endif

				fclose(file);
				return 0;
			}
		}
		
		fclose(file);

		return 1;
	}
	
	return 0;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUNÇÕES: ADICIONADAS
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int vc_rgb_to_gray(IVC *src, IVC *dst){
    unsigned char *datasrc = (unsigned char *) src->data;
    int bytesperline_src = src->width * src->channels;
    int channels_src = src->channels;
    unsigned char *datadst = (unsigned  char *) dst->data;
    int bytesperline_dst = dst->width * dst->channels;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x,y;
    long int pos_src, pos_dst;
    float rf, gf, bf;

    //verificação de erros
    if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if((src->width != dst->width) || (src->height != dst->height)) return 0;
    if((src->channels != 3) || (dst->channels != 1)) return 0;


    for (y=0; y<height; y++)
    {
        for (x=0; x<width; x++)
        {
            pos_src = y * bytesperline_src + x * channels_src;
            pos_dst = y * bytesperline_dst + x * channels_dst;
            rf = (float) datasrc[pos_src];
            gf = (float) datasrc[pos_src + 1];
            bf = (float) datasrc[pos_src + 2];
            datadst[pos_dst] = (unsigned char) ((rf * 0.299) + (gf * 0.587) + (bf * 0.114));
        }
    }
    return 1;
}

int vc_rgb_to_hsv(IVC *src, IVC *dst){
    unsigned char *datasrc = (unsigned char *) src->data;
    int bytesperline_src = src->width * src->channels;
    int channels_src = src->channels;
    unsigned char *datadst = (unsigned  char *) dst->data;
    int bytesperline_dst = dst->width * dst->channels;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x,y;
    long int pos_src, pos_dst;
    float rf, gf, bf;
    int max3, min3;
    float value, sat, hue;

    //verificação de erros
    if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if((src->width != dst->width) || (src->height != dst->height)) return 0;
    if((src->channels != 3) || (dst->channels != 3)) return 0;

    for (y=0; y<height; y++)
    {
        for (x=0; x<width; x++)
        {
            pos_src = y * bytesperline_src + x * channels_src;
            pos_dst = y * bytesperline_dst + x * channels_dst;
            rf = (float) datasrc[pos_src];
            gf = (float) datasrc[pos_src + 1];
            bf = (float) datasrc[pos_src + 2];
            max3 = MAX3(rf,gf,bf);
            min3 = MIN3(rf,gf,bf);

            if(max3 == 0){
                hue = 0;
                sat = 0;
            }else if (max3 == min3){
                hue = 0;
                sat = 0;
            }else{
                //----Value-----
                value = max3;
                //----Sat-----
                sat = (max3 - min3) / value;
                //----Hue------
                if(rf == max3){
                    if(gf >= bf ){
                        hue = (60 * (gf - bf)) / (max3 - min3);
                    }else if(bf > gf){
                        hue = 360 + (60 * (gf - bf) / (max3 - min3));
                    }
                }else if (gf == max3){
                    hue = 120 + (60 * (bf - rf) / (max3 -min3));
                }else if (bf == max3){
                    hue = 240 + (60 * (rf - gf) / (max3 - min3));
                }
            }

            datadst[pos_dst] = (hue / 360) * 255 ;
            datadst[pos_dst + 1 ] = sat * 255 ;
            datadst[pos_dst + 2] = value;
        }
    }
    
    return 1;

}
int vc_scale_gray_to_color_palette(IVC *src, IVC *dst){
    unsigned char *datasrc = (unsigned char *) src->data;
    int bytesperline_src = src->width * src->channels;
    int channels_src = src->channels;
    unsigned char *datadst = (unsigned  char *) dst->data;
    int bytesperline_dst = dst->width * dst->channels;
    int channels_dst = dst->channels;
    int width = src->width;
    int height = src->height;
    int x,y;
    long int pos_src, pos_dst;
    float bf, gf, rf;

    //verificação de erros
    if((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if((src->width != dst->width) || (src->height != dst->height)) return 0;
    if((src->channels != 1) || (dst->channels != 3)) return 0;


    for (y=0; y<height; y++)
    {
        for (x=0; x<width; x++)
        {
            pos_src = y * bytesperline_src + x * channels_src;
            pos_dst = y * bytesperline_dst + x * channels_dst;

            if( datasrc[pos_src] < 64 ){
                datadst[pos_dst] = 0;
                datadst[pos_dst + 1 ] = datasrc[pos_src] * 4 ;
                datadst[pos_dst + 2 ] = 255;
            }else if( datasrc[pos_src] < 128 ){
                datadst[pos_dst] = 0;
                datadst[pos_dst + 1 ] = 255;
                datadst[pos_dst + 2 ] = 255 - (datasrc[pos_src] - 64 )  * 4;
            }else if( datasrc[pos_src] < 192 ){
                datadst[pos_dst] = 255 - (datasrc[pos_src] - 128 ) * 4 ;
                datadst[pos_dst + 1 ] = 255;
                datadst[pos_dst + 2 ] = 0;
            }else{
                datadst[pos_dst] = 255;
                datadst[pos_dst + 1 ] = 255 - (datasrc[pos_src] - 192)  * 4;
                datadst[pos_dst + 2 ] = 0;
            }
        }
    }

    return 1;
}

int vc_gray_to_binary(IVC *src, IVC *dst, int threshold) {
    int x, y;
    int channels_src = src->channels;
    int  bystersperline_src = src->width * src->channels;
    long int pos;
    unsigned char *data_src = (unsigned char *)src->data;
    unsigned char *data_dst = (unsigned char *)dst->data;

    // Verificação de erros
    if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
    if ((src->width != dst->width) || (src->height != dst->height)) return 0;
    if ((src->channels != 1) || (dst->channels != 1)) return 0;

    for (y = 0; y < src->height; y++) {
        for (x = 0; x < src->width; x++) {
            pos = y * bystersperline_src + x * channels_src;

            if (data_src[pos] > threshold) {
                data_dst[pos] = 255; // Pixel acima do threshold, marcado como branco
            } else {
                data_dst[pos] = 0; // Pixel no ou abaixo do threshold, marcado como preto
            }
        }
    }

    return 1; // Sucesso
}
int vc_gray_to_binary_mean_threshold(IVC *src, IVC *dst) {
    unsigned char *data_src = (unsigned char *)src->data;
    int width = src->width;
    int height = src->height;
    long int sum = 0, pos;
    float mean_threshold;

    // Calcula a soma das intensidades de todos os pixels
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pos = y * width + x;
            sum += data_src[pos];
        }
    }

    // Calcula a média (threshold)
    mean_threshold = (float)sum / (float)(width * height);

    // Aplica o threshold e segmenta a imagem em binário
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            pos = y * width + x;
            data_src[pos] = (data_src[pos] > mean_threshold) ? 255 : 0;
        }
    }

    // Se dst for diferente de src, copie os dados para dst
    if (dst != src) {
        if ((dst->width != src->width) || (dst->height != src->height) || (dst->channels != 1)) return 0;
        memcpy(dst->data, src->data, width * height);
    }

    return 1; // Sucesso
}

int vc_gray_midpoint_threshold(IVC *src, IVC *dst, int kernel) {
    unsigned char *data_src = (unsigned char *)src->data;
    int width = src->width;
    int height = src->height;
    int x, y, wx, wy;
    int half_kernel = kernel / 2;
    unsigned char min_val, max_val;
    unsigned char pixel_val;

    if (src->channels != 1 || dst->channels != 1) {
        printf("ERROR: Both source and destination images must be grayscale.\n");
        return 0;
    }

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            min_val = 255;
            max_val = 0;

            // Percorre a vizinhança do pixel
            for (wy = -half_kernel; wy <= half_kernel; wy++) {
                for (wx = -half_kernel; wx <= half_kernel; wx++) {
                    int nx = x + wx;
                    int ny = y + wy;

                    // Verifica se está dentro dos limites da imagem
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
                        pixel_val = data_src[ny * width + nx];
                        if (pixel_val < min_val) min_val = pixel_val;
                        if (pixel_val > max_val) max_val = pixel_val;
                    }
                }
            }

            // Calcula o limiar para o pixel atual e aplica o threshold
            unsigned char threshold = (min_val + max_val) / 2;
            if (data_src[y * width + x] > threshold) {
                dst->data[y * width + x] = 0; // Branco
            } else {
                dst->data[y * width + x] = 255; // Preto
            }
        }
    }

    return 1; // Sucesso
}




