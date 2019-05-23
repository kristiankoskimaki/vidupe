/*
Modified from "C++ Implementation to Compare Pairs of Image Quality using RMSE, SSIM, and PSNR"
https://github.com/ruofeidu/ImageQualityCompare
Copyright (c) 2018 Ruofei Du (MIT License)
*/

#include "comparison.h"

using namespace cv;

double Comparison::sigma(const Mat &m, const int &i, const int &j, const int &block_size) const {
    const Mat m_tmp = m(Range(i, i + block_size), Range(j, j + block_size));
    const Mat m_squared(block_size, block_size, CV_32F);

    multiply(m_tmp, m_tmp, m_squared);

    const double avg = mean(m_tmp)[0];          //E(x)
    const double avg_2 = mean(m_squared)[0];    //E(x*x)
    const double sd = sqrt(avg_2 - avg * avg);

    return sd;
}

double Comparison::covariance(const Mat &m0, const Mat &m1, const int &i, const int &j, const int &block_size) const {
    const Mat m3 = Mat::zeros(block_size, block_size, CV_32F);
    const Mat m0_tmp = m0(Range(i, i + block_size), Range(j, j + block_size));
    const Mat m1_tmp = m1(Range(i, i + block_size), Range(j, j + block_size));

    multiply(m0_tmp, m1_tmp, m3);

    const double avg_ro = mean(m3)[0];              //E(XY)
    const double avg_r = mean(m0_tmp)[0];           //E(X)
    const double avg_o = mean(m1_tmp)[0];           //E(Y)
    const double sd_ro = avg_ro - avg_o * avg_r;    //E(XY) - E(X)E(Y)

    return sd_ro;
}

double Comparison::ssim(const Mat &m0, const Mat &m1, const int &block_size) const {
    double ssim = 0;
    const int nbBlockPerHeight = m0.rows / block_size;
    const int nbBlockPerWidth = m0.cols / block_size;
    constexpr double C1 = 0.01 * 255 * 0.01 * 255;
    constexpr double C2 = 0.03 * 255 * 0.03 * 255;

    for(int k=0; k<nbBlockPerHeight; k++) {
        for(int l=0; l<nbBlockPerWidth; l++) {
            const int m = k * block_size;
            const int n = l * block_size;

            const double avg_o = mean(m0(Range(k, k + block_size), Range(l, l + block_size)))[0];
            const double avg_r = mean(m1(Range(k, k + block_size), Range(l, l + block_size)))[0];
            const double sigma_o = sigma(m0, m, n, block_size);
            const double sigma_r = sigma(m1, m, n, block_size);
            const double sigma_ro = covariance(m0, m1, m, n, block_size);

            ssim += ((2 * avg_o * avg_r + C1) * (2 * sigma_ro + C2)) /
                    ((avg_o * avg_o + avg_r * avg_r + C1) * (sigma_o * sigma_o + sigma_r * sigma_r + C2));
        }
    }

    ssim = ssim / (nbBlockPerHeight * nbBlockPerWidth);
    return ssim;
}
