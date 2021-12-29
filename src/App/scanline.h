#pragma once

#include <opencv2/core/core.hpp>
#include <vector>
#include <list>

//ɨ�����㷨
class ScanLine
{
public:
	ScanLine(std::vector<cv::Point2f> corner_coordinate_);
	~ScanLine();
	//void SetPolygon(QPolygon apolygon);
	void FillPolygon(cv::Mat& mat);

private:
	void BuildEdgeTable();
	void UpdateActiveEdgeTable(int height);
	void CalcIntersect(int height);

private:
	struct Edge
	{
		int ymax;
		double dx;
		double x;

		inline bool operator<(const Edge& e2) const
		{
			return x < e2.x;
		}
	};

	int top_;
	int bottom_;
	int left_;
	int right_;
	int width_;
	int height_;

	std::vector<cv::Point2f> polygon_;

	std::vector<std::list<Edge>>	edge_table_;				//�߱�
	std::list<Edge>					active_edge_table_;		//���Ա߱�
	std::vector<double>				intersect_list_;				//����
//public:
//
//	static cv::Rect CalcRectRegion(const QPolygon& apolygon);
//	static cv::Rect CalcRectRegion(const QPolygon& apolygon, int awidth, int aheight);
//	static QPolygon ShiftPolygon(const QPolygon& apolygon, QPoint avec);
};