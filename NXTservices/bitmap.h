//
//  bitmap.h
//  xcode
//
//  Created by jl777 on 7/25/14.
//  Copyright (c) 2014 jl777. All rights reserved.
//

#ifndef xcode_bitmap_h
#define xcode_bitmap_h


double calc_loganswer(double pastlogprice,double futurelogprice)
{
	if ( fabs(pastlogprice) < .0000001 || fabs(futurelogprice) < .0000001 )
		return(0);
	return(10000. * (exp(futurelogprice - pastlogprice)-1.));
}

float _calc_pricey(double price,double weekave)
{
	if ( price != 0. && weekave != 0. )
		return(.25 * calc_loganswer(weekave,price));
	else
		return(0.f);
}

float pixelwt(int32_t color)
{
	return(((float)((color>>16)&0x0ff) + (float)((color>>8)&0x0ff) + (float)((color>>0)&0x0ff))/0x300);
}

int32_t pixel_ratios(uint32_t red,uint32_t green,uint32_t blue)
{
	float max;
	/*if ( red > green )
	 max = red;
	 else
	 max = green;
	 if ( blue > max )
	 max = blue;*/
	max = (red + green + blue);
	if ( max == 0. )
		return(0);
	if ( max > 0xff )
	{
		red = (uint32_t)(((float)red / max) * 0xff);
		green = (uint32_t)(((float)green / max) * 0xff);
		blue = (uint32_t)(((float)blue / max) * 0xff);
	}
	
	if ( red > 0xff )
		red = 0xff;
	if ( green > 0xff )
		green = 0xff;
	if ( blue > 0xff )
		blue = 0xff;
	return((red << 16) | (green << 8) | blue);
}

int32_t conv_yval_to_y(float yval,int32_t height)
{
	int32_t y;
	height = (height>>1) - 2;
	y = (int32_t)-yval;
	if ( y > height )
		y = height;
	else if ( y < -height )
		y = -height;
	
	y += height;
	if ( y < 0 )
		y = 0;
	height <<= 1;
	if ( y >= height-1 )
		y = height-1;
	return(y);
}

uint32_t scale_color(uint32_t color,float strength)
{
	int32_t red,green,blue;
	if ( strength < 0. )
		strength = -strength;
	red = (color>>16) & 0xff;
	green = (color>>8) & 0xff;
	blue = color & 0xff;
	
	red = (int32_t)((float)red * (strength/100.));
	green = (int32_t)((float)green * (strength/100.));
	blue = (int32_t)((float)blue * (strength/100.));
	if ( red > 0xff )
		red = 0xff;
	if ( green > 0xff )
		green = 0xff;
	if ( blue > 0xff )
		blue = 0xff;
	return((red<<16) | (green<<8) | blue);
}

uint32_t pixel_blend(uint32_t pixel,uint32_t color)//,int32_t groupsize)
{
	int32_t red,green,blue,sum,n,n2,groupsize = 1;
	float red2,green2,blue2,sum2;
	if ( color == 0 )
		return(pixel);
	if ( pixel == 0 )
	{
		return((1<<24) | scale_color(color,100./(float)groupsize));
	}
	n = (pixel>>24) & 0xff;
	if ( n == 0 )
		n = 1;
	pixel &= 0xffffff;
	red = (pixel>>16) & 0xff;
	green = (pixel>>8) & 0xff;
	blue = pixel & 0xff;
	sum = red + green + blue;
	
	n2 = (color>>24) & 0xff;
	if ( n2 == 0 )
		n2 = 1;
	red2 = ((float)((color>>16) & 0xff)) / groupsize;
	green2 = ((float)((color>>8) & 0xff)) / groupsize;
	blue2 = ((float)(color & 0xff)) / groupsize;
	sum2 = (red2 + green2 + blue2);
	
	//printf("gs %d (%d x %d,%d,%d: %d) + (%d x %.1f,%.1f,%.1f: %.1f) = ",groupsize,n,red,green,blue,sum,n2,red2,green2,blue2,sum2);
	red = (uint32_t)(((((((float)red / (float) sum) * n) + (((float)red2 / (float) sum2) * n2)) / (n+n2)) * ((sum+sum2)/2)));
	green = (uint32_t)(((((((float)green / (float) sum) * n) + (((float)green2 / (float) sum2) * n2)) / (n+n2)) * ((sum+sum2)/2)));
	blue = (uint32_t)(((((((float)blue / (float) sum) * n) + (((float)blue2 / (float) sum2) * n2)) / (n+n2)) * ((sum+sum2)/2)));
	
	n += n2;
	if ( n > 0xff )
		n = 0xff;
	///printf("%x (%d,%d,%d) ",color,red,green,blue);
	color = (n<<24) | pixel_ratios(red,green,blue);//pixel_overflow(&red,&green,&blue);
	
	//printf("%x (%d,%d,%d)\n",color,(color>>16)&0xff,(color>>8)&0xff,color&0xff);
	return(color);
}

void init_forex_colors()
{
	int32_t i;
	forex_colors[0] = 0x00ff00;
	forex_colors[1] = 0x0033ff;
	forex_colors[2] = 0xff0000;
	forex_colors[3] = 0x00ffff;
	forex_colors[4] = 0xffff00;
	forex_colors[5] = 0xff00ff;
	forex_colors[6] = 0xffffff;
	forex_colors[7] = 0xff8800;
	forex_colors[8] = 0xff88ff;
	for (i=9; i<16; i++)
		forex_colors[i] = pixel_blend(forex_colors[i-8],0xffffff);
}

int32_t is_primary_color(int32_t color)
{
	int32_t i;
	for (i=0; i<8; i++)
		if ( color == forex_colors[i] )
			return(1);
	return(0);
}

void disp_yval(int32_t color,float yval,uint32_t *bitmap,int32_t x,int32_t rowwidth,int32_t height)
{
	int32_t y;
	x += LEFTMARGIN;
	if ( x < 0 || x >= rowwidth )
		return;
	//y = conv_yval_to_y(yval,height/Display_scale) * Display_scale;
	y = conv_yval_to_y(yval * Display_scale,height);
	if ( 1 && is_primary_color(color) != 0 )
	{
		bitmap[y*rowwidth + x] = color;
		return;
	}
	//if ( pixelwt(color) > pixelwt(bitmap[y*rowwidth + x]) )
	bitmap[y*rowwidth + x] = pixel_blend(bitmap[y*rowwidth + x],color);
	return;
	if ( is_primary_color(color) != 0 || (is_primary_color(bitmap[y*rowwidth+x]) == 0 && pixelwt(color) > pixelwt(bitmap[y*rowwidth + x])) )
		bitmap[y*rowwidth + x] = color;
}

void disp_yvalsum(int32_t color,float yval,uint32_t *bitmap,int32_t x,int32_t rowwidth,int32_t height)
{
    int32_t y,red,green,blue,dispcolor;
	x += LEFTMARGIN;
	if ( x < 0 || x >= rowwidth )
		return;
	y = conv_yval_to_y(yval * Display_scale,height);
	red = (color>>16) & 0xff;
	green = (color>>8) & 0xff;
	blue = color & 0xff;
    dispcolor = bitmap[y*rowwidth + x];
	red += (dispcolor>>16) & 0xff;
	green += (dispcolor>>8) & 0xff;
	blue += dispcolor & 0xff;
	bitmap[y*rowwidth + x] = pixel_ratios(red,green,blue);
}

void disp_dot(float radius,int32_t color,float yval,uint32_t *bitmap,int32_t x,int32_t rowwidth,int32_t height)
{
	float i,j,sq,val;
	if ( radius > 1 )
	{
		sq = radius * radius;
		for (i=-radius; i<=radius; i++)
		{
			for (j=-radius; j<=radius; j++)
			{
				val = ((j*j + i*i) / sq);
				if ( val <= 1. )
				{
					val = 1. - val;
					disp_yval(scale_color(color,(100 * val * val * val * val)),yval+j,bitmap,x+i,rowwidth,height);
				}
			}
		}
	}
	else
		disp_yval(color,yval,bitmap,x,rowwidth,height);
}

void horizline(int32_t calclogflag,uint32_t *bitmap,double rawprice,double ave)
{
    int32_t x;
    double yval;
    if ( calclogflag != 0 )
        yval = _calc_pricey(log(rawprice),log(ave));
    else yval = _calc_pricey(rawprice,ave);
    for (x=0; x<Screenwidth; x++)
        disp_yval(0x888888,yval,bitmap,x,Screenwidth,Screenheight);
}

void rescale_floats(float *line,int32_t width,double scale)
{
    int32_t i;
    for (i=0; i<width; i++)
        line[i] *= scale;
}

void rescale_doubles(double *line,int32_t width,double scale)
{
    int32_t i;
    for (i=0; i<width; i++)
        line[i] *= scale;
}

double _output_line(int32_t calclogflag,double ave,double *output,double *buf,int32_t n,int32_t color,uint32_t *bitmap,int32_t rowwidth,int32_t height)
{
    int32_t x,nonz = 0;
    double yval,val,aveabs = 0.;
    if ( ave == 0. )
        return(0.);
    if ( calclogflag != 0 )
        ave = log(ave);
    for (x=0; x<n; x++)
    {
        if ( (val= buf[x]) != 0 )
        {
            if ( calclogflag != 0 )
                val = log(buf[x]);
            if ( ave != 1. )
                yval = _calc_pricey(val,ave);
            else yval = val;
            printf("%f ",yval);
            if ( fabs(yval) > .0000000001 )
            {
                aveabs += fabs(yval);
                nonz++;
                if ( color != 0 )
                    disp_yval(color,yval,bitmap,x,rowwidth,height);
            }
        } else yval = 0.;
        output[x] = yval;
    }
    if ( nonz != 0 )
        aveabs /= nonz;
    return(aveabs);
    //
    //printf("ave %f rowwidth.%d\n",ave,rowwidth);
}

double output_line(int32_t smoothiters,int32_t calclogflag,double ave,double *output,double *input,int32_t n,int32_t color,uint32_t *bitmap,int32_t rowwidth,int32_t height)
{
    double src[2048],dest[2048];
    int32_t i;
    memset(src,0,sizeof(src));
    memset(dest,0,sizeof(dest));
    if ( smoothiters != 0 )
    {
        void smooth1024(double dest[],double src[],int32_t smoothiters);
        for (i=0; i<1024; i++)
            src[1023-i] = dest[1023-i] = input[i];
        smooth1024(dest,src,smoothiters);
        for (int32_t i=0; i<1024; i++)
            src[1023-i] = dest[i];
    }
    else
    {
        for (i=0; i<n; i++)
        {
            src[i] = input[i];
            printf("%f ",src[i]);
        }
    }
    return(_output_line(calclogflag,ave,output,src,rowwidth,color,bitmap,rowwidth,height));
}

int32_t nonzrow(uint32_t *bitmap,int32_t rowwidth)
{
    int32_t i;
    for (i=0; i<rowwidth; i++)
        if ( bitmap[i] != 0 )
            return(1);
    return(0);
}

void gen_ppmfile(char *fname,int32_t rows,int32_t startx,int32_t endx,uint32_t *bitmap,int32_t rowwidth,int32_t height)
{
    int32_t x,y,j,color;
    FILE *fp;
    /*
     Each PPM image consists of the following:
     
     A "magic number" for identifying the file type. A ppm image's magic number is the two characters "P6".
     Whitespace (blanks, TABs, CRs, LFs).
     A width, formatted as ASCII characters in decimal.
     Whitespace.
     A height, again in ASCII decimal.
     Whitespace.
     The maximum color value (Maxval), again in ASCII decimal. Must be less than 65536 and more than zero.
     A single whitespace character (usually a newline).
     A raster of Height rows, in order from top to bottom. Each row consists of Width pixels, in order from left to right. Each pixel is a triplet of red, green, and blue samples, in that order. Each sample is represented in pure binary by either 1 or 2 bytes. If the Maxval is less than 256, it is 1 byte. Otherwise, it is 2 bytes. The most significant byte is first.
     */
    if ( (fp= fopen(fname,"wb")) != 0 )
    {
        if ( rows < 0 )
            y = height/2 + rows, rows = -rows * 2;
        else if ( rows == 0 )
        {
            y = 0;
            for (j=0; j<height; j++)
            {
                if ( nonzrow(bitmap+rowwidth*j,rowwidth) != 0 )
                {
                    y = j - 5;
                    break;
                }
            }
            if ( y < 0 )
                y = 0;
            for (j=height-1; j>y+10; j--)
            {
                if ( nonzrow(bitmap+rowwidth*j,rowwidth) != 0 )
                    break;
            }
            rows = (j - y);
        }
        else y = 0;
        fprintf(fp,"P6 %d %d 255\n",(endx - startx + 1),rows);
        for (j=0; j<rows; j++,y++)
            for (x=startx; x<=endx; x++)
            {
                color = bitmap[y*rowwidth + x];
                fputc((color >> 16) & 0xff,fp);
                fputc((color >> 8) & 0xff,fp);
                fputc((color >> 0) & 0xff,fp);
            }
        fclose(fp);
    }
}

void output_jpg(char *name,uint32_t *bitmap,int32_t width,int32_t height)
{
    //int32_t calclogflag = 0;
    char fname[512],cmd[512];
    /*float bids[1024],asks[1024];
    memset(bids,0,sizeof(bids));
    memset(asks,0,sizeof(asks));
    memset(bitmap,0,sizeof(*bitmap) * Screenwidth*Screenheight);
    horizline(calclogflag,bitmap,ave,ave);
    output_line(3,calclogflag,ave,bids,1024,0x0088ff,bitmap,Screenwidth,Screenheight);
    output_line(3,calclogflag,ave,asks,1024,0xff3300,bitmap,Screenwidth,Screenheight);*/
    sprintf(fname,"%s.ppm",name);
    gen_ppmfile(fname,height,0,width-1,bitmap,width,height);
    sprintf(cmd,"convert %s %s.jpg",fname,name);
    system(cmd);
}

int32_t Numimages,*Image_sizes;
unsigned char **Image_buffers; char **Image_names;

int32_t search_images(unsigned char *testimage,char *arg)
{
    int32_t i;
    if ( Numimages == 0 )
        return(0);
    //printf("search (%s)\n",arg);
    for (i=0; i<Numimages; i++)
    {
        if ( strcmp(arg,Image_names[i]) == 0 && Image_sizes[i] != 0 )
        {
            memcpy(testimage,Image_buffers[i],Image_sizes[i]);
            return(Image_sizes[i]);
        }
    }
    return(0);
}

int32_t add_image(char *imagename)
{
    char *clonestr(char *);
    FILE *fp;
    char fname[512];
    int32_t i = 0,imagesize;
    sprintf(fname,"%s.jpg",imagename);
    if ( Numimages > 0 )
    {
        for (i=0; i<Numimages; i++)
        {
            if ( strcmp(Image_names[i],fname) == 0 )
                break;
        }
    }
    if ( i == Numimages )
    {
        Numimages++;
        Image_buffers = realloc(Image_buffers,sizeof(*Image_buffers) * Numimages);
        Image_names = realloc(Image_names,sizeof(*Image_names) * Numimages);
        Image_sizes = realloc(Image_sizes,sizeof(*Image_sizes) * Numimages);
        Image_names[i] = clonestr(fname);
        Image_sizes[i] = 0;
        Image_buffers[i] = calloc(1024*1024,sizeof(int32_t)); // should be safe to have 4MB buffer
        printf("allocate image.%d\n",i);
    }
    printf("loading %s image.%d of %d\n",fname,i,Numimages);
    imagesize = 0;
    if ( (fp= fopen(fname,"rb")) != 0 )
    {
        fseek(fp,0,SEEK_END);
        imagesize = (int32_t)ftell(fp);
        rewind(fp);
        if ( fread(Image_buffers[i],1,imagesize,fp) != imagesize )
            printf("error reading %s %d bytes\n",Image_names[i],imagesize);
        fclose(fp);
    }
    Image_sizes[i] = imagesize;
    return(imagesize);
}

#endif
