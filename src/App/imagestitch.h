#pragma once

#include <opencv2/highgui.hpp>    
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/xfeatures2d.hpp>
#include <vector>
#include <string>
#include <algorithm>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/photo.hpp>
#include <cmath>


class ImageStitcher
{
public:
	ImageStitcher(cv::Mat image_current_, cv::Mat image_past_, std::string blending_method_);
	~ImageStitcher();
	void MakeStitching();

private:
	void FeaturePointMap();
	void ImageAlignment();
	void ImageFusionSpecial();
	void ImageFusionNormal();
	std::vector<cv::Point2f> CornerTransform(cv::Mat image_, cv::Mat homograpgy_matrix_);

private:
	cv::Mat image_c_;
	cv::Mat image_p_;
	cv::Mat image_transform_p_;								//��ȥͼ�񾭹�ͶӰ�任��Ľ��
	std::string b_method_;									//NONE|WEIGHT_AVG|POISSON
	std::vector<cv::Point2f> feature_point_list_c_;			//��ǰͼ����������б�
	std::vector<cv::Point2f> feature_point_list_p_;			//��ȥͼ����������б�
	std::vector<cv::Point2f> corner_transform_list_p_;      //��ȥͼ����ĸ�����ͶӰ����任��������б�
	cv::Mat image_gray_c_;
	cv::Mat image_gray_p_;

public:
	cv::Mat fused_image_;
	
};
