#include "imagestitch.h"
#include <opencv2/highgui.hpp>    
#include <opencv2/opencv.hpp>
#include <string>
#include <iostream>


int main(int argc, char* argv[])
{
    /*if (argc != 3)
    {
        std::cout << "Please input the current image (left) and past image (right) in terminal" << std::endl;
        exit(-1);
    }*/

    //std::string image_current_path_ = argv[1];
    //std::string image_past_path_ = argv[2];
    std::string image_current_path_ = "C:/Users/12239/Desktop/LookIntoPast/test/1/current.jpg";
    std::string image_past_path_ = "C:/Users/12239/Desktop/LookIntoPast/test/1/past.jpg";
    size_t common_prefix_length_ = 0;
    /*std::cout << "The current image is " << image_current_path_ << std::endl;
    std::cout << "The past image is " << image_past_path_ << std::endl;*/
    cv::Mat image_current_ = cv::imread(image_current_path_);                        //��õ�ǰ��ͼ��
    cv::Mat image_past_ = cv::imread(image_past_path_);                              //��ù�ȥ��ͼ��

    ImageStitcher image_stitcher_none_(image_current_, image_past_, "NONE");         //����ͼ��ƴ����(NONE-���ںϷ���)
    ImageStitcher image_stitcher_avg_(image_current_, image_past_, "WEIGHT_AVG");    //����ͼ��ƴ����(WEIGHT_AVG-Ȩ���ںϷ���)
    ImageStitcher image_stitcher_poisson_(image_current_, image_past_, "POISSON");   //����ͼ��ƴ����(POISSON-�����ںϷ���)

    /*imshow("The current image", image_current_);
    imshow("The past image", image_past_);

    cv::waitKey();*/

    //image_stitcher_none_.MakeStitching();
    image_stitcher_avg_.MakeStitching();
    //image_stitcher_poisson_.MakeStitching();
    //cv::imshow("The fusion image (NONE)", image_stitcher_none_.fused_image_);
    //cv::imshow("The fusion image (WEIGHT_AVG)", image_stitcher_avg_.fused_image_);
    //cv::imshow("The fusion image (POISSON)", image_stitcher_poisson_.fused_image_);

    for (size_t i = 0; i < image_current_path_.length(); i++)
    {
        if (image_current_path_[i] != image_past_path_[i])
        {
            common_prefix_length_ = i;
            break;
        }
    }

    //����ͼƬ���
    //cv::imwrite(image_current_path_.substr(0, common_prefix_length_) + "FusionResult_NONE.png", image_stitcher_none_.fused_image_);
    cv::imwrite(image_current_path_.substr(0, common_prefix_length_) + "FusionResult_WEIGHT_AVG.png", image_stitcher_avg_.fused_image_);
    //cv::imwrite(image_current_path_.substr(0, common_prefix_length_) + "FusionResult_POISSON.png", image_stitcher_poisson_.fused_image_);


	return 0;
}