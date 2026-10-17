// name=spider.c
// 简单程序：生成 800x600 的蜘蛛图像，输出 spider.ppm (P6)
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

typedef unsigned char u8;
typedef struct { int w, h; u8 *data; } Image;

Image *create_image(int w, int h, u8 r, u8 g, u8 b) {
    Image *img = malloc(sizeof(Image));
    img->w = w; img->h = h;
    img->data = malloc(3 * w * h);
    for (int i = 0; i < w*h; ++i) {
        img->data[3*i+0] = r;
        img->data[3*i+1] = g;
        img->data[3*i+2] = b;
    }
    return img;
}

void set_pixel(Image *img, int x, int y, u8 r, u8 g, u8 b) {
    if (x < 0 || x >= img->w || y < 0 || y >= img->h) return;
    int idx = 3 * (y * img->w + x);
    img->data[idx+0] = r;
    img->data[idx+1] = g;
    img->data[idx+2] = b;
}

void save_ppm(Image *img, const char *filename) {
    FILE *f = fopen(filename, "wb");
    if (!f) { perror("fopen"); return; }
    fprintf(f, "P6\n%d %d\n255\n", img->w, img->h);
    fwrite(img->data, 3, img->w * img->h, f);
    fclose(f);
}

// 简单填充圆
void fill_circle(Image *img, int cx, int cy, int r, u8 rr, u8 gg, u8 bb) {
    int r2 = r*r;
    for (int y = cy - r; y <= cy + r; ++y) {
        for (int x = cx - r; x <= cx + r; ++x) {
            int dx = x - cx, dy = y - cy;
            if (dx*dx + dy*dy <= r2) set_pixel(img, x, y, rr, gg, bb);
        }
    }
}

// Bresenham 线
void draw_line(Image *img, int x0, int y0, int x1, int y1, u8 r, u8 g, u8 b) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    while (1) {
        set_pixel(img, x0, y0, r, g, b);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2*err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// 粗线：多偏移画多条线以增宽
void draw_thick_line(Image *img, int x0, int y0, int x1, int y1, int thickness, u8 r, u8 g, u8 b) {
    if (thickness <= 1) { draw_line(img, x0, y0, x1, y1, r, g, b); return; }
    // 计算法线方向（单位化）
    double dx = x1 - x0, dy = y1 - y0;
    double len = sqrt(dx*dx + dy*dy);
    double nx = 0, ny = 0;
    if (len != 0) { nx = -dy / len; ny = dx / len; }
    int half = thickness/2;
    for (int t = -half; t <= half; ++t) {
        int ox = (int)round(nx * t);
        int oy = (int)round(ny * t);
        draw_line(img, x0 + ox, y0 + oy, x1 + ox, y1 + oy, r, g, b);
    }
}

int main(void) {
    const int W = 800, H = 600;
    Image *img = create_image(W, H, 255, 255, 255); // 白底

    // 蜘蛛位置与尺寸
    int cx = W/2, cy = H/2 + 30;
    int abdomen_r = 80;
    int ceph_r = 48;
    u8 spider_r = 30, spider_g = 30, spider_b = 30; // 黑暗颜色

    // 身体：腹部与头胸部
    fill_circle(img, cx, cy + 40, abdomen_r, spider_r, spider_g, spider_b);
    fill_circle(img, cx - 10, cy - 10, ceph_r, spider_r, spider_g, spider_b);

    // 画腹部高光（椭圆高光）
    for (int y = cy + 40 - abdomen_r; y <= cy + 40 + abdomen_r; ++y) {
        for (int x = cx - abdomen_r; x <= cx + abdomen_r; ++x) {
            double dx = (x - (cx-20)) / (double)abdomen_r;
            double dy = (y - (cy+20)) / (double)(abdomen_r*0.7);
            double v = dx*dx + dy*dy;
            if (v <= 1.0 && rand()%150==0) { // 少量随机高光点，增强纹理
                set_pixel(img, x, y, 70, 70, 70);
            }
        }
    }

    // 画眼睛（两个白点和黑瞳）
    fill_circle(img, cx - 28, cy - 20, 6, 255, 255, 255);
    fill_circle(img, cx - 8, cy - 26, 6, 255, 255, 255);
    fill_circle(img, cx - 28, cy - 20, 2, 0, 0, 0);
    fill_circle(img, cx - 8, cy - 26, 2, 0, 0, 0);

    // 画腿：8 条（每侧4条），使用两段线模拟关节
    int legs_each_side = 4;
    double baseR = ceph_r + 4;
    int leg_len1 = 90, leg_len2 = 70;
    int thickness = 4;
    for (int side = -1; side <= 1; side += 2) { // -1 左, +1 右
        for (int i = 0; i < legs_each_side; ++i) {
            // 分布角度：从上到下，大致范围
            double t = (double)i / (legs_each_side - 1); // 0..1
            double ang = ( -M_PI/3.0 + t * (M_PI/3.0) ) * side; // 左右对称
            // 起点在头胸部边缘
            int x0 = cx + (int)round(cos(ang) * baseR);
            int y0 = cy + (int)round(sin(ang) * baseR) - 10;
            // 第一段终点（略向外）
            double ang1 = ang + (side * (0.15 + 0.05*i)); // slight outward bend
            int x1 = x0 + (int)round(cos(ang1) * leg_len1);
            int y1 = y0 + (int)round(sin(ang1) * leg_len1);
            // 第二段终点（再折一点）
            double ang2 = ang1 + (side * (0.25 + 0.03*i));
            int x2 = x1 + (int)round(cos(ang2) * leg_len2);
            int y2 = y1 + (int)round(sin(ang2) * leg_len2);

            draw_thick_line(img, x0, y0, x1, y1, thickness, spider_r, spider_g, spider_b);
            draw_thick_line(img, x1, y1, x2, y2, thickness-1, spider_r, spider_g, spider_b);

            // 画小关节圆点
            fill_circle(img, x1, y1, 4, spider_r, spider_g, spider_b);
        }
    }

    // 在腹部加点竖条花纹
    for (int k = -2; k <= 2; ++k) {
        int rx = cx + k * 12;
        for (int y = cy + 0; y <= cy + 80; ++y) {
            int dx = rx - cx;
            int dy = y - (cy + 40);
            if (dx*dx + (int)(dy*0.6)*(int)(dy*0.6) < 1400) {
                if ((y + k*7) % 7 < 4) set_pixel(img, rx, y, 10, 10, 10);
            }
        }
    }

    // 保存文件
    save_ppm(img, "spider.ppm");
    printf("生成 spider.ppm（%dx%d）完成。\n", W, H);

    free(img->data);
    free(img);
    return 0;
}   //添加蜘蛛侠代码
