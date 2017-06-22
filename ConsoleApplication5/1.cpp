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
	OFCondition oc = fileformat.loadFile(file_path.c_str());                    //��ȡDicomͼ��    
	if (!oc.good())     //�ж�Dicom�ļ��Ƿ��ȡ�ɹ�    
	{
		std::cout << "file Load error" << std::endl;
		return nullptr;
	}
	DcmDataset *dataset = fileformat.getDataset();                              //�õ�Dicom�����ݼ�    
	E_TransferSyntax xfer = dataset->getOriginalXfer();                          //�õ������﷨    

	OFString patientname;
	dataset->findAndGetOFString(DCM_PatientName, patientname);                   //��ȡ��������    

	unsigned short bit_count(0);
	dataset->findAndGetUint16(DCM_BitsStored, bit_count);                        //��ȡ���ص�λ�� bit    

	OFString isRGB;
	dataset->findAndGetOFString(DCM_PhotometricInterpretation, isRGB);           //DCMͼƬ��ͼ��ģʽ    

	unsigned short img_bits(0);
	dataset->findAndGetUint16(DCM_SamplesPerPixel, img_bits);                    //��������ռ�ö���byte    
	Img_bitCount = (int)img_bits;

	OFString framecount;
	dataset->findAndGetOFString(DCM_NumberOfFrames, framecount);             //DCMͼƬ��֡��    


	//DicomImage* img_xfer = new DicomImage(xfer, 0, 0, 1);                     //�ɴ����﷨�õ�ͼ���֡    

	unsigned short m_width;                                                     //��ȡͼ��Ĵ����    
	unsigned short m_height;
	dataset->findAndGetUint16(DCM_Rows, m_height);
	dataset->findAndGetUint16(DCM_Columns, m_width);

	/////////////////////////////////////////////////////////////////////////    
	const char* transferSyntax = NULL;
	fileformat.getMetaInfo()->findAndGetString(DCM_TransferSyntaxUID, transferSyntax);       //��ô����﷨�ַ���    
	string losslessTransUID = "1.2.840.10008.1.2.4.70";
	string lossTransUID = "1.2.840.10008.1.2.4.51";
	string losslessP14 = "1.2.840.10008.1.2.4.57";
	string lossyP1 = "1.2.840.10008.1.2.4.50";
	string lossyRLE = "1.2.840.10008.1.2.5";
	if (transferSyntax == losslessTransUID || transferSyntax == lossTransUID ||
		transferSyntax == losslessP14 || transferSyntax == lossyP1)
	{
		DJDecoderRegistration::registerCodecs();
		dataset->chooseRepresentation(EXS_LittleEndianExplicit, NULL);                       //��ѹ����ͼ�����ؽ��н�ѹ    
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

	DicomImage* m_dcmImage = new DicomImage((DcmObject*)dataset, xfer); //����dataset����DicomImage����Ҫ����Ľ�ѹ������    

	return m_dcmImage;
}

std::vector<cv::Mat> GetImageFromDcmDataSet(DicomImage* m_dcmImage)
{
	std::vector<cv::Mat> output_img;              //���ͼ������  
	int framecount(m_dcmImage->getFrameCount()); //��ȡ����ļ�������ͼ���֡��  
	for (int k = 0; k<framecount; k++)
	{
		unsigned char *pixelData = (unsigned char*)(m_dcmImage->getOutputData(8, k, 0)); //���8λ��ͼ������ָ��    
		if (pixelData != NULL)
		{
			int m_height = m_dcmImage->getHeight();
			int m_width = m_dcmImage->getWidth();
			cout << "�߶ȣ�" << m_height << "������" << m_width << endl;
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

	std::string file_path = "C:\\Users\\lidaboo\\Desktop\\CT.dcm";             //dcm�ļ�  

	DicomImage *img = new DicomImage(file_path.c_str());
	if (img->isMonochrome() && img->getStatus() == EIS_Normal && img != NULL)
	{
		if (img->isMonochrome())
		{
			int nWidth = img->getWidth();            //���ͼ����  
			int nHeight = img->getHeight();          //���ͼ��߶�  

			Uint16 *pixelData = (Uint16*)(img->getOutputData(16));   //���16λ��ͼ������ָ��  
			std::cout << nWidth << ", " << nHeight << std::endl;
			if (pixelData != NULL)
			{
				//����1  
				/*IplImage *srcImage = cvCreateImageHeader(cvSize(nWidth, nHeight), IPL_DEPTH_16U, 1);
				cvSetData(srcImage, pixelData, nWidth*sizeof(unsigned short));
				cv::Mat dst(srcImage);
				cv::imshow("image1", dst);*/

				//����2  
				cv::Mat dst2(nWidth, nHeight, CV_16UC1, cv::Scalar::all(0));
				unsigned short* data = nullptr;
				for (int i = 0; i < nHeight; i++)
				{
					data = dst2.ptr<unsigned short>(i);   //ȡ��ÿһ�е�ͷָ�� Ҳ��ʹ��dst2.at<unsigned short>(i, j) = ?  
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