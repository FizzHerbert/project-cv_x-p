#include "imagestitch.h"
#include "scanline.h"
#include "poissonfusion.h"


ImageStitcher::ImageStitcher(cv::Mat image_current_, cv::Mat image_past_, std::string blending_method_)
{
	image_c_ = image_current_;
	image_p_ = image_past_;
    b_method_ = blending_method_;
}


ImageStitcher::~ImageStitcher()
{
}


void ImageStitcher::MakeStitching()
{
    size_t background_pixel_ = 0;
    size_t background_ratio_ = image_p_.rows * image_p_.cols / 10;

    FeaturePointMap();
    ImageAlignment();

    for (size_t i = 0; i < image_gray_p_.rows; i++)
    {
        if (background_pixel_ > background_ratio_) //����1/10�����ص㱻�ж�Ϊ����������Ϊ��ͼƬ������յȵ�ɫ����
        {
            break;
        }
        for (size_t j = 0; j < image_gray_p_.cols; j++)
        {
            if (image_gray_p_.at<uchar>(i, j) >= 200)
            {
                background_pixel_++;
            }
        }
    }

    std::cout << background_pixel_ << std::endl;

    if (background_pixel_ < background_ratio_) //���ʱ����Ϊͼ���б������٣�ʹ�������ı�Ե�ں�
    {
        std::cout << "�ںϷ�ʽ:Normal" << std::endl;
        ImageFusionNormal();
    }
    else //���ʱ����Ϊͼ�����н϶൭ɫ�������Ա������⴦��
    {
        std::cout << "�ںϷ�ʽ:Special" << std::endl;
        ImageFusionSpecial();
    }
}


void ImageStitcher::FeaturePointMap()
{
    //SIFT�����������    100, 3, 0.07, 5, 2.4(image pairs 2) / 0, 3, 0.12, 7, 2.8 (image pairs 1)
    auto sift_detector_ = cv::SIFT::create(0, 3, 0.12, 7, 2.8);
    std::vector<cv::KeyPoint> keypoint_list_c_, keypoint_list_p_;                                
    cv::Mat descriptor_c_, descriptor_p_;
    cv::FlannBasedMatcher point_matcher_;      
    //cv::BFMatcher point_matcher_;
    //std::vector<std::vector<cv::DMatch>> matched_point_;
    std::vector<cv::DMatch> well_matched_point_list_;                                            //ƥ�����õ���������б�
    cv::Mat image_match_;
    /*cv::Mat image_laplac_c_, image_laplac_p_;
    cv::Mat image_blur_c_, image_blur_p_;

    cv::GaussianBlur(image_c_, image_blur_c_, cv::Size(3, 3), 5, 0);
    cv::Laplacian(image_blur_c_, image_laplac_c_, CV_16S, 3, 1, 0, cv::BORDER_DEFAULT);
    cv::convertScaleAbs(image_laplac_c_, image_laplac_c_);
    cv::add(image_c_, image_laplac_c_, image_laplac_c_);
    imshow("current�ݶ�ͼ��", image_laplac_c_);
    cv::GaussianBlur(image_p_, image_blur_p_, cv::Size(3, 3), 2, 0);
    cv::Laplacian(image_blur_p_, image_laplac_p_, CV_16S, 3, 1, 0, cv::BORDER_DEFAULT);
    cv::convertScaleAbs(image_laplac_p_, image_laplac_p_);
    cv::add(image_p_, image_laplac_p_, image_laplac_p_);
    imshow("past�ݶ�ͼ��", image_laplac_p_);
    cv::waitKey();*/

    cvtColor(image_c_, image_gray_c_, cv::COLOR_RGB2GRAY);                                       //��õ�ǰͼ��ĻҶ�ͼ
    cvtColor(image_p_, image_gray_p_, cv::COLOR_RGB2GRAY);                                       //��ù�ȥͼ��ĻҶ�ͼ
    //cvtColor(image_laplac_c_, image_gray_c_, cv::COLOR_RGB2GRAY);                                       //��õ�ǰͼ��ĻҶ�ͼ
    //cvtColor(image_laplac_p_, image_gray_p_, cv::COLOR_RGB2GRAY);                                       //��ù�ȥͼ��ĻҶ�ͼ

    sift_detector_->detectAndCompute(image_gray_c_, cv::Mat(), keypoint_list_c_, descriptor_c_);      //��ǰͼ���������������
    sift_detector_->detectAndCompute(image_gray_p_, cv::Mat(), keypoint_list_p_, descriptor_p_);      //��ȥͼ���������������  
    //sift_detector_->detectAndCompute(image_laplac_c_, cv::Mat(), keypoint_list_c_, descriptor_c_);      //��ǰͼ���������������
    //sift_detector_->detectAndCompute(image_laplac_p_, cv::Mat(), keypoint_list_p_, descriptor_p_);      //��ȥͼ��������������� 
    point_matcher_.match(descriptor_p_, descriptor_c_, well_matched_point_list_);
    /*point_matcher_.add(std::vector<cv::Mat>(1, descriptor_c_));
    point_matcher_.train();
    point_matcher_.knnMatch(descriptor_p_, matched_point_, 2);
    for (size_t i = 0; i < matched_point_.size(); i++)
    {
        if (matched_point_[i][0].distance < 0.75f * matched_point_[i][1].distance)
        {
            well_matched_point_list_.push_back(matched_point_[i][0]);
        }
    }*/
    //std::cout << "The total match points' number is:" << well_matched_point_list_.size() << std::endl;

    std::sort(well_matched_point_list_.begin(), well_matched_point_list_.end());
    well_matched_point_list_.resize(1);

    drawMatches(image_p_, keypoint_list_p_, image_c_, keypoint_list_c_, well_matched_point_list_, image_match_);
    imshow("The match image ", image_match_);
    cv::waitKey();

    for (int i = 0; i < 1; i++)                                    //��ȡ����ͼ��ֱ��Ӧ����������(����ֻȡƥ��Ч����õ��Ǹ�) 
    {
        feature_point_list_p_.push_back(keypoint_list_p_[well_matched_point_list_[i].queryIdx].pt);
        feature_point_list_c_.push_back(keypoint_list_c_[well_matched_point_list_[i].trainIdx].pt);
    }
}



void ImageStitcher::ImageAlignment()
{
    cv::Mat homography_matrix_;                             //��ȥͼ�񵽵�ǰͼ��ı任����(��С3*3)����Ϊ����ƥ����������������٣��㲻��ͶӰ����������ƽ�ƾ������

    homography_matrix_ = (cv::Mat_<double>(3, 3) << 1, 0, (int)(feature_point_list_c_[0].x - feature_point_list_p_[0].x), 0, 1, (int)(feature_point_list_c_[0].y - feature_point_list_p_[0].y), 0, 0, 1.0);
    //homography_matrix_ = findHomography(feature_point_list_p_, feature_point_list_c_, cv::RANSAC);
    //std::cout << "�任����Ϊ��\n" << homography_matrix_ << std::endl << std::endl;

    corner_transform_list_p_ = CornerTransform(image_p_, homography_matrix_);   //�����ȥͼ����ĸ�����任�������
    /*std::cout << "left_top:" << corner_transform_list_p_[0] << std::endl;
    std::cout << "left_bottom:" << corner_transform_list_p_[1] << std::endl;
    std::cout << "right_top:" << corner_transform_list_p_[2] << std::endl;
    std::cout << "right_bottom:" << corner_transform_list_p_[3] << std::endl;*/

    //ͼ����׼  
    warpPerspective(image_p_, image_transform_p_, homography_matrix_, cv::Size(image_c_.cols, image_c_.rows));
    /*image_transform_p_ = cv::Mat::zeros(image_c_.rows, image_c_.cols, CV_8UC3);
    for (size_t i = ; i < )*/
    /*cv::imshow("��ȥͼ�񾭹�͸�Ӿ���任��Ľ��", image_transform_p_);
    cv::waitKey();*/
    //cv::imwrite("trans1.jpg", imageTransform1);

    //ImageFusion(image_c_, image_transform_p_, fused_image_);


    //cv::imshow("�ںϺ��ͼ��", fused_image_);

    //cv::waitKey();
}


void ImageStitcher::ImageFusionSpecial()
{
    fused_image_ = cv::Mat::zeros(image_c_.rows, image_c_.cols, CV_8UC3);       //�����ںϵõ���ͼ��
    //fused_image_.setTo(0);
    //ScanLine scanline_ = ScanLine(corner_transform_list_p_);
    //scanline_.FillPolygon(mask_matrix_);
    cv::Mat mask_matrix_ = cv::Mat::zeros(image_c_.rows, image_c_.cols, CV_8UC1);

    for (size_t i = MAX(1, corner_transform_list_p_[0].y), max_row = MIN(image_c_.rows - 1, corner_transform_list_p_[3].y); i < max_row; i++)
    {
        for (size_t j = MAX(1, corner_transform_list_p_[0].x), max_col = MIN(image_c_.cols - 1, corner_transform_list_p_[3].x); j < max_col; j++)
        {
            mask_matrix_.at<uchar>(i, j) = 1;
        }
    }
    
    if (b_method_ == "NONE")
    {
        image_c_.copyTo(fused_image_(cv::Rect(0, 0, image_c_.cols, image_c_.rows)));
        //image_transform_p_.copyTo(fused_image_(cv::Rect(MAX(0, corner_transform_list_p_[0].x), MAX(0, corner_transform_list_p_[0].y), image_transform_p_.cols, image_transform_p_.rows)));
        image_transform_p_.copyTo(fused_image_, mask_matrix_);
             
    }
    else if (b_method_ == "WEIGHT_AVG")
    {
        double weight_;
        std::vector<std::vector<double>> edge_pixel_weight_;
        cv::Point2i center_transform_p_;
        cv::Mat image_gray_transform_p_;
        size_t pixel_num_ = 40;


        cvtColor(image_transform_p_, image_gray_transform_p_, cv::COLOR_RGB2GRAY);

        center_transform_p_.x = (int)((MIN(image_c_.cols, corner_transform_list_p_[3].x) + MAX(0, corner_transform_list_p_[0].x)) / 2);
        center_transform_p_.y = (int)((MIN(image_c_.rows, corner_transform_list_p_[3].y) + MAX(0, corner_transform_list_p_[0].y)) / 2);
        image_c_.copyTo(fused_image_(cv::Rect(0, 0, image_c_.cols, image_c_.rows)));

        for (size_t i = 0; i < 3; i++)
        {
            std::vector<double> edge_pixel_temp_;
            edge_pixel_weight_.push_back(edge_pixel_temp_);//�ĸ�Ȩ�طֱ��¼image_c_��Ե30�����ϵ�Ȩ��ֵ��˳��Ϊ���Ե|�ϱ�Ե|�ұ�Ե|�±�Ե
        }

        for (size_t i = MAX(0, corner_transform_list_p_[0].y), max_row = MIN(image_c_.rows, corner_transform_list_p_[3].y); i < max_row; i++)
        {
            for (size_t j = MAX(0, corner_transform_list_p_[0].x), max_col = MIN(image_c_.cols, corner_transform_list_p_[3].x); j < max_col; j++)
            {
                cv::Point2d center_distance_, max_center_distance_;
                size_t index = 4;

                center_distance_ = cv::Point2d(std::abs(center_transform_p_.x - (int)j), std::abs(center_transform_p_.y - (int)i));
                //std::cout << center_distance_ << std::endl;
                max_center_distance_ = cv::Point2d(center_transform_p_.x - MAX(1, corner_transform_list_p_[0].x), center_transform_p_.y - MAX(1, corner_transform_list_p_[0].y));
                //std::cout << max_center_distance_ << std::endl;
                weight_ = MAX(center_distance_.x / max_center_distance_.x, center_distance_.y / max_center_distance_.y);
                //weight_ = MAX(center_distance_.x / (center_distance_.x + 20), center_distance_.y / (center_distance_.y + 20));
                if (image_gray_transform_p_.at<uchar>(i, j) >= 210)
                {
                    weight_ = std::pow(weight_, 1);
                }
                else if (image_c_.rows - i < pixel_num_ || image_c_.cols - j < pixel_num_ || i < pixel_num_ || j < pixel_num_)
                {
                    continue;
                }
                else
                {
                    weight_ = std::pow(weight_, 20);              
                }

                if (j == pixel_num_)
                {
                    index = 0;
                }
                if (i == pixel_num_)
                {
                    index = 1;
                }
                if (image_c_.cols - j == pixel_num_)
                {

                    index = 2;
                }
                if (image_c_.rows - i == pixel_num_)
                {
                    index = 3;
                }

                if (index < 4)
                {
                    edge_pixel_weight_[index].push_back(weight_);
                }
                //std::cout << "weight" << weight_ << std::endl;
                fused_image_.at<cv::Vec3b>(i, j) = weight_ * image_c_.at<cv::Vec3b>(i, j) + (1 - weight_) * image_transform_p_.at<cv::Vec3b>(i, j);
            }
        }
        if (MAX(0, corner_transform_list_p_[0].x) < pixel_num_)
        {
            for (size_t i = MAX(0, corner_transform_list_p_[0].x); i < pixel_num_; i++)
            {
                for (size_t j = 0; j < edge_pixel_weight_[0].size(); j++)
                {
                    size_t row_index_ = j + MAX(0, corner_transform_list_p_[0].y);

                    if (image_gray_transform_p_.at<uchar>(row_index_, i) >= 210)
                    {
                        continue;
                    }
                    weight_ = edge_pixel_weight_[0][j];
                    fused_image_.at<cv::Vec3b>(row_index_, i) = weight_ * image_c_.at<cv::Vec3b>(row_index_, i) + (1 - weight_) * image_transform_p_.at<cv::Vec3b>(row_index_, i);
                }
            }
        }
        if (MAX(0, corner_transform_list_p_[0].y) < pixel_num_)
        {
            for (size_t i = MAX(0, corner_transform_list_p_[0].y); i < pixel_num_; i++)
            {
                for (size_t j = 0; j < edge_pixel_weight_[1].size(); j++)
                {
                    size_t col_index_ = j + MAX(0, corner_transform_list_p_[0].x);

                    if (image_gray_transform_p_.at<uchar>(i, col_index_) >= 210)
                    {
                        continue;
                    }
                    weight_ = edge_pixel_weight_[1][j];
                    fused_image_.at<cv::Vec3b>(i, col_index_) = weight_ * image_c_.at<cv::Vec3b>(i, col_index_) + (1 - weight_) * image_transform_p_.at<cv::Vec3b>(i, col_index_);
                }
            }
        }
        if (image_c_.cols - MIN(image_c_.cols, corner_transform_list_p_[3].x) < pixel_num_)
        {
            for (size_t i = image_c_.cols - pixel_num_; i < MIN(image_c_.cols, corner_transform_list_p_[3].x); i++)
            {
                for (size_t j = 0; j < edge_pixel_weight_[2].size(); j++)
                {
                    size_t row_index_ = j + MAX(0, corner_transform_list_p_[0].y);

                    if (image_gray_transform_p_.at<uchar>(row_index_, i) >= 210)
                    {
                        continue;
                    }
                    weight_ = edge_pixel_weight_[2][j];
                    fused_image_.at<cv::Vec3b>(row_index_, i) = weight_ * image_c_.at<cv::Vec3b>(row_index_, i) + (1 - weight_) * image_transform_p_.at<cv::Vec3b>(row_index_, i);                  
                }
            }
        }
        if (image_c_.rows - MIN(image_c_.rows, corner_transform_list_p_[3].y) < pixel_num_)
        {       
            for (size_t i = image_c_.rows - pixel_num_; i < MIN(image_c_.cols, corner_transform_list_p_[3].y); i++)
            {
                for (size_t j = 0; j < edge_pixel_weight_[3].size(); j++)
                {
                    size_t col_index_ = j + MAX(0, corner_transform_list_p_[0].x);

                    if (image_gray_transform_p_.at<uchar>(i, col_index_) >= 210)
                    {
                        continue;
                    }
                    weight_ = edge_pixel_weight_[3][j];
                    fused_image_.at<cv::Vec3b>(i, col_index_) = weight_ * image_c_.at<cv::Vec3b>(i, col_index_) + (1 - weight_) * image_transform_p_.at<cv::Vec3b>(i, col_index_);
                }
            }
        }
    }
    else if (b_method_ == "POISSON")
    {
        std::cout << "Poissom method" << std::endl;
        PoissonFusion poisson_editor_;

        poisson_editor_.SetSourceMask(image_transform_p_, mask_matrix_);
        poisson_editor_.SetMixedGrad(false);
        poisson_editor_.SetTarget(image_c_);
        poisson_editor_.SetShiftPos(cv::Point(0, 0));
        
        //cv::seamlessClone(image_transform_p_, image_c_, mask_matrix_, cv::Point(image_c_.cols / 2 , image_c_.rows / 2), fused_image_, cv::NORMAL_CLONE);

        fused_image_ = poisson_editor_.GetFusion();
    }
    else
    {
        std::cout << "��֧������ͼ���ںϷ�ʽ(��ѡ��NONE��WEIGHT_AVG��POISSON)" << std::endl;
    }

    cv::imshow("ƴ��ͼ��", fused_image_);
    cv::waitKey();
}


void ImageStitcher::ImageFusionNormal()
{
    fused_image_ = cv::Mat::zeros(image_c_.rows, image_c_.cols, CV_8UC3);       //�����ںϵõ���ͼ��
    //fused_image_.setTo(0);
    //ScanLine scanline_ = ScanLine(corner_transform_list_p_);
    //scanline_.FillPolygon(mask_matrix_);
    cv::Mat mask_matrix_ = cv::Mat::zeros(image_c_.rows, image_c_.cols, CV_8UC1);

    for (size_t i = MAX(1, corner_transform_list_p_[0].y), max_row = MIN(image_c_.rows - 1, corner_transform_list_p_[3].y); i < max_row; i++)
    {
        for (size_t j = MAX(1, corner_transform_list_p_[0].x), max_col = MIN(image_c_.cols - 1, corner_transform_list_p_[3].x); j < max_col; j++)
        {
            mask_matrix_.at<uchar>(i, j) = 1;
        }
    }

    if (b_method_ == "NONE")
    {
        image_c_.copyTo(fused_image_(cv::Rect(0, 0, image_c_.cols, image_c_.rows)));
        //image_transform_p_.copyTo(fused_image_(cv::Rect(MAX(0, corner_transform_list_p_[0].x), MAX(0, corner_transform_list_p_[0].y), image_transform_p_.cols, image_transform_p_.rows)));
        image_transform_p_.copyTo(fused_image_, mask_matrix_);

    }
    else if (b_method_ == "WEIGHT_AVG")
    {
        double weight_;
        cv::Point2i center_transform_p_;


        center_transform_p_.x = (int)((MIN(image_c_.cols, corner_transform_list_p_[3].x) + MAX(0, corner_transform_list_p_[0].x)) / 2);
        center_transform_p_.y = (int)((MIN(image_c_.rows, corner_transform_list_p_[3].y) + MAX(0, corner_transform_list_p_[0].y)) / 2);
        image_c_.copyTo(fused_image_(cv::Rect(0, 0, image_c_.cols, image_c_.rows)));

        for (size_t i = MAX(0, corner_transform_list_p_[0].y), max_row = MIN(image_c_.rows, corner_transform_list_p_[3].y); i < max_row; i++)
        {
            for (size_t j = MAX(0, corner_transform_list_p_[0].x), max_col = MIN(image_c_.cols, corner_transform_list_p_[3].x); j < max_col; j++)
            {
                cv::Point2d center_distance_, max_center_distance_;

                center_distance_ = cv::Point2d(std::abs(center_transform_p_.x - (int)j), std::abs(center_transform_p_.y - (int)i));
                //std::cout << center_distance_ << std::endl;
                max_center_distance_ = cv::Point2d(center_transform_p_.x - MAX(1, corner_transform_list_p_[0].x), center_transform_p_.y - MAX(1, corner_transform_list_p_[0].y));
                //std::cout << max_center_distance_ << std::endl;
                weight_ = MAX(center_distance_.x / max_center_distance_.x, center_distance_.y / max_center_distance_.y);
                //weight_ = MAX(center_distance_.x / (center_distance_.x + 20), center_distance_.y / (center_distance_.y + 20));
                weight_ = std::pow(weight_, 7);              
                //std::cout << "weight" << weight_ << std::endl;
                fused_image_.at<cv::Vec3b>(i, j) = weight_ * image_c_.at<cv::Vec3b>(i, j) + (1 - weight_) * image_transform_p_.at<cv::Vec3b>(i, j);
            }
        }
    }
    else if (b_method_ == "POISSON")
    {
        std::cout << "Poissom method" << std::endl;
        PoissonFusion poisson_editor_;

        poisson_editor_.SetSourceMask(image_transform_p_, mask_matrix_);
        poisson_editor_.SetMixedGrad(false);
        poisson_editor_.SetTarget(image_c_);
        poisson_editor_.SetShiftPos(cv::Point(0, 0));

        //cv::seamlessClone(image_transform_p_, image_c_, mask_matrix_, cv::Point(image_c_.cols / 2 , image_c_.rows / 2), fused_image_, cv::NORMAL_CLONE);

        fused_image_ = poisson_editor_.GetFusion();
    }
    else
    {
        std::cout << "��֧������ͼ���ںϷ�ʽ" << std::endl;
    }

    cv::imshow("ƴ��ͼ��", fused_image_);
    cv::waitKey();
}


std::vector<cv::Point2f> ImageStitcher::CornerTransform(cv::Mat image_, cv::Mat homograpgy_matrix_)
{
    float width_ = image_.cols;
    float height_ = image_.rows;
    //ͼ���ĸ��ǵĳ�ʼ����,0��(���Ͻ�)/1��(���½�)/2��(���Ͻ�)/3��(���½�)
    cv::Mat corners_init_matrix_ = (cv::Mat_<double>(3, 4) << 0, 0, width_, width_, 0, height_, 0, height_, 1.0, 1.0, 1.0, 1.0);
    cv::Mat corners_transform_matrix_;                      //ͼ���ĸ��Ǳ任�������������
    std::vector<cv::Point2f> corners_transform_list_;       //ͼ���ĸ��Ǳ任��������б�

    //std::cout << "�Ǿ���\n" << corners_init_matrix_ << std::endl;

    corners_transform_matrix_ = homograpgy_matrix_ * corners_init_matrix_;

    //std::cout << "�任��ĽǾ���\n" << corners_transform_matrix_ << std::endl;

    for (size_t i = 0; i < corners_transform_matrix_.cols; i++)
    {
        cv::Point2f corner_coordinate;
        corner_coordinate.x = corners_transform_matrix_.at<double>(0, i) / corners_transform_matrix_.at<double>(2, i);
        corner_coordinate.y = corners_transform_matrix_.at<double>(1, i) / corners_transform_matrix_.at<double>(2, i);
        corners_transform_list_.push_back(corner_coordinate);
    }

    return corners_transform_list_;
}



