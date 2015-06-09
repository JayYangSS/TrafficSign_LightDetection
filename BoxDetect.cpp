#include"traffic.h"
void BoxDetect(Mat src_test,HOGDescriptor &myHOG,vector<Rect> &found_filtered)
{
	vector<Rect> found;
	cout<<"进行多尺度HOG交通灯检测"<<endl;
	myHOG.detectMultiScale(src_test, found,0.5,Size(4,4), Size(0,0), 1.05, 2);//对图片进行多尺度行人检测
    cout<<"找到的矩形框个数："<<found.size()<<endl;

  //找出所有没有嵌套的矩形框r,并放入found_filtered中,如果有嵌套的话,则取外面最大的那个矩形框放入found_filtered中
  for(int i=0; i < found.size(); i++)
  {
    Rect r = found[i];
    int j=0;
    for(; j < found.size(); j++)
      if(j != i && (r & found[j]) == r)
        break;
    if( j == found.size())
      found_filtered.push_back(r);
  }

  //画矩形框，因为hog检测出的矩形框比实际人体框要稍微大些,所以这里需要做一些调整
  for(int i=0; i<found_filtered.size(); i++)
  {
    Rect r = found_filtered[i];

    r.x += cvRound(r.width*0.1);
    r.width = cvRound(r.width*0.8);
    r.y += cvRound(r.height*0.07);
    r.height = cvRound(r.height*0.8);
   // rectangle(src_test, r.tl(), r.br(), Scalar(0,255,0), 1);
	rectangle(src_test, r.tl(), r.br(), Scalar(0,255,0), 2);//tl()返回左上角点坐标,br()返回右下角点坐标
  }

	imshow("src",src_test);
 
	waitKey(20);//注意：imshow之后必须加waitKey，否则无法显示图像
}