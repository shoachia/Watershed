﻿#include <opencv2//core//core.hpp>
#include <opencv2//highgui//highgui.hpp>
#include <iostream>
#include <string>
#include "opencv2/imgproc/imgproc.hpp"
#include <stdio.h>
#include <iomanip>
#include <opencv2/opencv.hpp>
using namespace cv;
using namespace std;

Point before_pos(-1,-1);
Point aft_pos(-1,-1);
Point new_pos(0,0);

void onMouse(int Event,int x,int y,int flags,void* param);
void antiThresh(Mat &src, Mat &dst, int Thresh);
void doWatershed();
void myWatershed(Mat src, Mat gray, int thres);
void resizeimg_show(Mat src,Size dsize,string name);
void showpart_img(Mat src,int x_start,int y_start,Size window,string name);
void google_shift(Mat src,string name);
Mat LBP(Mat src);


int main()
{
	
	Mat lena, cameraman, myPic, stainedcell_large, stainedcell_small;

	cout << "Watershed result" << endl;
	doWatershed();

	waitKey(0);
	return 0;
}


void antiThresh(Mat &src, Mat &dst, int Thresh)
{
	dst = src.clone();
	for(int y = 0; y < (dst.rows); y++)
	{
		for(int x = 0; x < dst.cols; x++)
		{
			if(dst.at<uchar>(y,x) < Thresh)
				dst.at<uchar>(y,x) = 255;
			else
				dst.at<uchar>(y,x) = 0;
		}
	}
};


void doWatershed()
{
	Mat img, gray_img;
	img = imread("stainedcell_large.jpg", 1);
	//img = imread("cell_1.jpg", 1);
	//img = imread("3.jpg", 1);
	cvtColor(img, gray_img, CV_BGR2GRAY);
	myWatershed(img, gray_img,110);
	waitKey(0);
}

void myWatershed(Mat src, Mat gray, int thres)
{
	Mat gray_b;
	Mat r_src;
	Mat r_dst;
	Mat lbp;
	Mat part_img1,part_img2;
	blur(gray,gray_b,Size(5,5));
	antiThresh(gray_b, gray_b, thres);
    Mat binary;
	
    gray_b.convertTo(binary, CV_8U);
    // Find total markers
    vector<vector<Point> > contours;
    findContours(binary, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);//findContours
	//contours.size()=1179 for large cells
    // Create the marker image for the watershed algorithm
	/**********************************create a histogram package************************************************/
	vector<vector<int>>histo_stat;
	histo_stat.resize(contours.size());
	for(int n=0;n<contours.size();n++)
	{
		histo_stat[n].resize(256);
	}
	for(int i=0;i<contours.size();i++)
	{
		for(int j=0;j<256;j++)
		{
			histo_stat[i][j]=0;
		}
	}
	/**********************************create a histogram package************************************************/
	/*************************************CALCULATE DESCRIPTOR***************************************************/
	double area=0.0;
	double length=0.0;
	double compact=0.0;
	double T=1;
	int count=0;
	vector<double> area_mat;
	vector<double> length_mat;
	vector<double> com_mat;
	double area_sum=0.0;
	double area_avg=0.0;
	double area_var=0.0;
	double area_temp=0.0;
	double length_sum=0.0;
	double length_avg=0.0;
	double length_var=0.0;
	double length_temp=0.0;
	double com_sum=0.0;
	double com_avg=0.0;
	double com_var=0.0;
	double com_temp=0.0;
	for(int i=0;i<contours.size();i++)
	{
		area=contourArea(contours[i],false);
		length=arcLength(contours[i],false);
		if(area==0)
		{compact=0;}//Avoid calculating the wrong components
		else
		{
			compact=pow(length,2)/area;}
		if(area<T){count++;}
		area_mat.push_back(area);
		length_mat.push_back(length);
		com_mat.push_back(compact);
		//cout<<"No. "<<setw(4)<<i+1<<",area= "<<setw(4)<<area<<" ,length= "<<setw(8)<<length<<" ,compactness= "<<setw(8)<<compact<<endl;
	}
	for(int j=0;j<contours.size();j++)
	{
		if((area_mat[j]>T)&&(length_mat[j]>T)&&(com_mat[j]>T))
		{
			length_sum=length_sum+length_mat[j];
			area_sum=area_sum+area_mat[j];
			com_sum=com_sum+com_mat[j];
		}
	}
	length_avg=length_sum/(contours.size()-count);
	area_avg=area_sum/(contours.size()-count);
	com_avg=com_sum/(contours.size()-count);
		
	for(int t=0;t<contours.size();t++)
	{
		if((area_mat[t]>T)&&(length_mat[t]>T)&&(com_mat[t]>T))
		{
			length_temp+=pow(length_mat[t]-length_avg,2);
			area_temp+=pow(area_mat[t]-area_avg,2);
			com_temp+=pow(com_mat[t]-com_avg,2);
		}
	}
	length_var=length_temp/(contours.size()-count);
	area_var=area_temp/(contours.size()-count);
	com_var=com_temp/(contours.size()-count);
	
	cout<<"Overall area variance level : "<<(sqrt(area_var)/area_avg)*100<<endl;//得到變異係數
	cout<<"Overall length variance level : "<<((sqrt(length_var))/length_avg)*100<<endl;
	cout<<"Overall shape variance level : "<<((sqrt(com_var))/com_avg)*100<<endl;
	
	/*************************************CALCULATE DESCRIPTOR***************************************************/


    Mat markers = Mat::zeros(gray.size(), CV_32SC1);
    // Draw the foreground markers
    for (size_t i = 0; i < contours.size(); i++)
        drawContours(markers, contours, static_cast<int>(i), Scalar::all(static_cast<int>(i)+1), -1);//drawContours
    // Draw the background marker
    circle(markers, Point(5,5), 3, CV_RGB(255,255,255), -1);
   // imshow("Markers & Contours", markers*10000);


	// Perform the watershed algorithm
    watershed(src, markers);//watershed
    Mat mark = Mat::zeros(markers.size(), CV_8UC1);
    markers.convertTo(mark, CV_8UC1);
    bitwise_not(mark, mark);
    //imshow("markers", markers); // uncomment this if you want to see how the mark
                                  // image looks like at that point
    // Generate random colors
    vector<Vec3b> colors;
    for (size_t i = 0; i < contours.size(); i++)
    {
        int b = theRNG().uniform(0, 255);
        int g = theRNG().uniform(0, 255);
        int r = theRNG().uniform(0, 255);
        colors.push_back(Vec3b((uchar)b, (uchar)g, (uchar)r));
    }
    // Create the result image
    Mat dst = Mat::zeros(markers.size(), CV_8UC3);
    // Fill labeled objects with random colors

    for (int i = 0; i < markers.rows; i++)
    {
        for (int j = 0; j < markers.cols; j++)
        {
            int index = markers.at<int>(i,j);

            if (index > 0 && index <= static_cast<int>(contours.size()))
			{
                dst.at<Vec3b>(i,j) = colors[index-1];
			}
            else
                dst.at<Vec3b>(i,j) = Vec3b(0,0,0);
        }
    }


	/**********************************Making LBP************************************************************/
	lbp=LBP(src);
	//resizeimg_show(lbp,Size(500,500),"LBP");
	
	for (int i = 0; i < lbp.rows; i++)
    {
        for (int j = 0; j < lbp.cols; j++)
        {
            int index = markers.at<int>(i,j);
			int graylevel=0;
			
            if (index > 0 && index <= static_cast<int>(contours.size()))
			{
				graylevel=lbp.at<uchar>(i,j);
				histo_stat[index-1][graylevel]+=1;
			} 
        }
    }

	int sum[256]={0};
	for(int a=0;a<256;a++)
	{
		for(int b=0;b<contours.size();b++)
		{
			sum[a]+=histo_stat[b][a];
		}
	}

	
	int histMaxValue=0;
	for(int p=0; p<256; p++)
	{
        int tempValue = sum[p];
        if(histMaxValue < tempValue)
		{
            histMaxValue = tempValue;
		}
	}
	double scale=(0.95*256)/histMaxValue;
	

	Mat histImage( 256, 256, CV_8U, Scalar(0) );
	for(int n=0;n<256;n++)
	{
		line(histImage,Point (n,255),Point (n,255-(sum[n])/contours.size()),Scalar(255));
	}
	imshow("cells characteristic stastic",histImage);
	/**********************************Making LBP************************************************************/


	/*********************putting text on image*************************************************************/
	for(int i=0;i<contours.size();i++)
	{
		stringstream s1,s2;
		s1<<i+1;
		s2<<area_mat[i];
		putText(dst,s1.str()+",area: "+s2.str(),contours[i][0],FONT_HERSHEY_COMPLEX ,0.3,Scalar(255,0,0));
		putText(src,s1.str(),contours[i][0],FONT_HERSHEY_COMPLEX ,0.3,Scalar(255,0,0));
	}

	/*********************putting text on image**************************************************************/


	resizeimg_show(src,Size(500,500),"origin");
	
	resizeimg_show(dst,Size(500,500),"final");
	google_shift(src,"ori");//using mouse to view the whole image
							//ESC to escape from google_shift function
	google_shift(dst,"fin");


	
	
	}

	void resizeimg_show(Mat src_i,Size dsize,string name)
	{
		Mat dst_i;
		resize(src_i,dst_i,dsize,0,0,INTER_LINEAR);
		imshow(name,dst_i);
	}
	void showpart_img(Mat src_s,int x_start,int y_start,Size window,string name)
	{
		Mat img_ROI;
		img_ROI=src_s(Rect(x_start,y_start,window.width,window.height));
		imshow(name,img_ROI);
	}
	void onMouse(int Event,int x,int y,int flags,void* param)
	{
		if(Event==CV_EVENT_LBUTTONDOWN)
		{
			before_pos.x=x;
			before_pos.y=y;
		}
		if(Event==CV_EVENT_LBUTTONUP)
		{
			aft_pos.x=x;
			aft_pos.y=y;
		}
	}
	void google_shift(Mat src_g,string name)
	{
		namedWindow(name,0);
	setMouseCallback(name,onMouse ,NULL);
	while(true)
	{
		if(before_pos.x==-1 && aft_pos.x==-1&&before_pos.y==-1 && aft_pos.y==-1)//mouse stay still
		{
           showpart_img(src_g,new_pos.x,new_pos.y,Size(500,500),name);
        }
		if((before_pos.x!=-1 && aft_pos.x!=-1)||(before_pos.y!=-1 && aft_pos.y!=-1))//if the mouse move
		{
				new_pos.x+=before_pos.x-aft_pos.x;
				if(new_pos.x<0)
					new_pos.x=0;
				else if(new_pos.x>src_g.cols-500)
					new_pos.x=src_g.cols-500;
				
			
				new_pos.y+=before_pos.y-aft_pos.y;
				if(new_pos.y<0)
					new_pos.y=0;
				else if(new_pos.y>src_g.rows-500)
					new_pos.y=src_g.rows-500;
				

			showpart_img(src_g,new_pos.x,new_pos.y,Size(500,500),name);

            before_pos.x = -1;
            before_pos.y = -1;
            aft_pos.x = -1;
            aft_pos.y = -1;
        }
		if(cvWaitKey(33)==27)
		{
            break;
        }
	}
	}
	Mat LBP(Mat src)
	{
		Mat gray(src.size(),CV_8UC1);
		Mat lbp(src.size(),CV_8UC1);
		if (src.channels() == 3)
        cvtColor(src, gray, CV_BGR2GRAY); 
		unsigned center=0;
		unsigned lbp_center=0;
		for (int r = 1; r < gray.rows-1; r++)            
		{
			for (int c = 1; c < gray.cols-1; c++)
			{
				center=gray.at<uchar>(r,c);
				lbp_center=0;
				//calculate the pixel clockwise and start with left-up pixel
				if(center<=gray.at<uchar>(r-1,c-1))
				{
					lbp_center+=1;
				}
				if(center<=gray.at<uchar>(r-1,c))
				{
					lbp_center+=2;
				}
				if(center<=gray.at<uchar>(r-1,c+1))
				{
					lbp_center+=4;
				}
				if(center<=gray.at<uchar>(r,c+1))
				{
					lbp_center+=8;
				}
				if(center<=gray.at<uchar>(r+1,c+1))
				{
					lbp_center+=16;
				}
				if(center<=gray.at<uchar>(r+1,c))
				{
					lbp_center+=32;
				}
				if(center<=gray.at<uchar>(r+1,c-1))
				{
					lbp_center+=64;
				}
				if(center<=gray.at<uchar>(r,c-1))
				{
					lbp_center+=128;
				}
				lbp.at<uchar>(r,c)=lbp_center;
			}
		}
		return lbp;
	}
