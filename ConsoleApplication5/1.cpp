#include<iostream>
#include<opencv2\opencv.hpp>

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dctk.h"
#include "dcmtk/dcmimgle/dcmimage.h"
#include "dcmtk/dcmjpeg/djdecode.h"
#include "dcmtk/dcmdata/dcrledrg.h"
#include "dcmtk/dcmimage/diregist.h"

using namespace std;
using namespace cv;

#pragma comment(lib, "Netapi32.lib")
#pragma comment(lib, "ws2_32.lib")

int Img_bitCount;

DicomImage* LoadDcmDataSet(std::string file_path)
{

	DcmFileFormat fileformat;
	OFCondition oc = fileformat.loadFile(file_path.c_str());                    //读取Dicom图像    
	if (!oc.good())     //判断Dicom文件是否读取成功    
	{
		std::cout << "file Load error" << std::endl;
		return nullptr;
	}
	DcmDataset *dataset = fileformat.getDataset();                              //得到Dicom的数据集    
	E_TransferSyntax xfer = dataset->getOriginalXfer();                          //得到传输语法    

	OFString patientname;
	dataset->findAndGetOFString(DCM_PatientName, patientname);                   //获取病人姓名    

	unsigned short bit_count(0);
	dataset->findAndGetUint16(DCM_BitsStored, bit_count);                        //获取像素的位数 bit    

	OFString isRGB;
	dataset->findAndGetOFString(DCM_PhotometricInterpretation, isRGB);           //DCM图片的图像模式    

	unsigned short img_bits(0);
	dataset->findAndGetUint16(DCM_SamplesPerPixel, img_bits);                    //单个像素占用多少byte    
	Img_bitCount = (int)img_bits;

	OFString framecount;
	dataset->findAndGetOFString(DCM_NumberOfFrames, framecount);             //DCM图片的帧数    


	//DicomImage* img_xfer = new DicomImage(xfer, 0, 0, 1);                     //由传输语法得到图像的帧    

	unsigned short m_width;                                                     //获取图像的窗宽高    
	unsigned short m_height;
	dataset->findAndGetUint16(DCM_Rows, m_height);
	dataset->findAndGetUint16(DCM_Columns, m_width);

	/////////////////////////////////////////////////////////////////////////    
	const char* transferSyntax = NULL;
	fileformat.getMetaInfo()->findAndGetString(DCM_TransferSyntaxUID, transferSyntax);       //获得传输语法字符串    
	string losslessTransUID = "1.2.840.10008.1.2.4.70";
	string lossTransUID = "1.2.840.10008.1.2.4.51";
	string losslessP14 = "1.2.840.10008.1.2.4.57";
	string lossyP1 = "1.2.840.10008.1.2.4.50";
	string lossyRLE = "1.2.840.10008.1.2.5";
	if (transferSyntax == losslessTransUID || transferSyntax == lossTransUID ||
		transferSyntax == losslessP14 || transferSyntax == lossyP1)
	{
		DJDecoderRegistration::registerCodecs();
		dataset->chooseRepresentation(EXS_LittleEndianExplicit, NULL);                       //对压缩的图像像素进行解压    
		DJDecoderRegistration::cleanup();
	}
	else if (transferSyntax == lossyRLE)
	{
		DcmRLEDecoderRegistration::registerCodecs();
		dataset->chooseRepresentation(EXS_LittleEndianExplicit, NULL);
		DcmRLEDecoderRegistration::cleanup();
	}
	else
	{
		dataset->chooseRepresentation(xfer, NULL);
	}

	DicomImage* m_dcmImage = new DicomImage((DcmObject*)dataset, xfer); //利用dataset生成DicomImage，需要上面的解压方法；    

	return m_dcmImage;
}

std::vector<cv::Mat> GetImageFromDcmDataSet(DicomImage* m_dcmImage)
{
	std::vector<cv::Mat> output_img;              //输出图像向量  
	int framecount(m_dcmImage->getFrameCount()); //获取这个文件包含的图像的帧数  
	for (int k = 0; k<framecount; k++)
	{
		unsigned char *pixelData = (unsigned char*)(m_dcmImage->getOutputData(8, k, 0)); //获得8位的图像数据指针    
		if (pixelData != NULL)
		{
			int m_height = m_dcmImage->getHeight();
			int m_width = m_dcmImage->getWidth();
			cout << "高度：" << m_height << "，长度" << m_width << endl;
			if (3 == Img_bitCount)
			{
				cv::Mat dst2(m_height, m_width, CV_8UC3, cv::Scalar::all(0));
				for (int i = 0; i < m_height; i++)
				{
					for (int j = 0; j < m_width; j++)
					{
						dst2.at<cv::Vec3b>(i, j)[0] = *(pixelData + i*m_width * 3 + j * 3 + 2);   //B channel    
						dst2.at<cv::Vec3b>(i, j)[1] = *(pixelData + i*m_width * 3 + j * 3 + 1);   //G channel    
						dst2.at<cv::Vec3b>(i, j)[2] = *(pixelData + i*m_width * 3 + j * 3);       //R channel    
					}
				}
				output_img.push_back(dst2);
			}
			else if (1 == Img_bitCount)
			{
				cv::Mat dst2(m_height, m_width, CV_8UC1, cv::Scalar::all(0));
				uchar* data = nullptr;
				for (int i = 0; i < m_height; i++)
				{
					data = dst2.ptr<uchar>(i);
					for (int j = 0; j < m_width; j++)
					{
						data[j] = *(pixelData + i*m_width + j);
					}
				}
				output_img.push_back(dst2);
			}

			/*cv::imshow("image", dst2);
			cv::waitKey(0);*/
		}
	}

	return output_img;
}

int main()
{
	Mat a = imread("C:\\Users\\lidaboo\\Desktop\\test.bmp");
	imshow("image", a);

	std::string file_path = "C:\\Users\\lidaboo\\Desktop\\CT.dcm";             //dcm文件  

	DicomImage *img = new DicomImage(file_path.c_str());
	if (img->isMonochrome() && img->getStatus() == EIS_Normal && img != NULL)
	{
		if (img->isMonochrome())
		{
			int nWidth = img->getWidth();            //获得图像宽度  
			int nHeight = img->getHeight();          //获得图像高度  

			Uint16 *pixelData = (Uint16*)(img->getOutputData(16));   //获得16位的图像数据指针  
			std::cout << nWidth << ", " << nHeight << std::endl;
			if (pixelData != NULL)
			{
				//方法1  
				/*IplImage *srcImage = cvCreateImageHeader(cvSize(nWidth, nHeight), IPL_DEPTH_16U, 1);
				cvSetData(srcImage, pixelData, nWidth*sizeof(unsigned short));
				cv::Mat dst(srcImage);
				cv::imshow("image1", dst);*/

				//方法2  
				cv::Mat dst2(nWidth, nHeight, CV_16UC1, cv::Scalar::all(0));
				unsigned short* data = nullptr;
				for (int i = 0; i < nHeight; i++)
				{
					data = dst2.ptr<unsigned short>(i);   //取得每一行的头指针 也可使用dst2.at<unsigned short>(i, j) = ?  
					for (int j = 0; j < nWidth; j++)
					{
						*data++ = pixelData[i*nWidth + j];
					}
				}
				cv::imshow("image2", dst2);
			}
		}
	}

	string filepath = "C:\\Users\\lidaboo\\Desktop\\5.dcm";
	DicomImage *m_dcmImage;
	m_dcmImage = LoadDcmDataSet(filepath);

	vector<cv::Mat> images;
	images = GetImageFromDcmDataSet(m_dcmImage);
	cout << images.size() << endl;
	imshow("image", images[1]);
	cv::waitKey(0);
	delete img;
	return 0;
}