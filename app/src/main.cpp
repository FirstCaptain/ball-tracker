#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <numeric>

using namespace std;
using namespace cv;

const int RIGHT = 1, LEFT = -1, ZERO = 0;

// What side of the line is a point
int directionOfPoint(Point A, Point B, Point P)
{
    B.x -= A.x;
    B.y -= A.y;
    P.x -= A.x;
    P.y -= A.y;

    int cross_product = B.x * P.y - B.y * P.x;

    if (cross_product > 0)
        return RIGHT;

    if (cross_product < 0)
        return LEFT;

    return ZERO;
}
//hough lines to find the X white lines
Mat find_regions(Mat input, Mat &output, Point2f &pt1, Point2f &pt2, Point2f &pt3, Point2f &pt4)
{
    Mat gray, thresh, dst;

    cvtColor(input, gray, COLOR_BGR2GRAY);
    // namedWindow("gray", WINDOW_NORMAL);
    // imshow("gray", gray);
    threshold(gray, thresh, 225, 255, 0);
    erode(thresh, thresh, getStructuringElement(MORPH_RECT, Size(3, 3), Point(1, 1)));
    erode(thresh, thresh, getStructuringElement(MORPH_RECT, Size(3, 3), Point(1, 1)));

    // namedWindow("thresh", WINDOW_NORMAL);
    // imshow("thresh", thresh);
    vector<Vec2f> lines;
    Canny(thresh, dst, 50, 200, 3);
    HoughLines(dst, lines, 1, CV_PI / 180, 200, 0, 0);

    std::vector<cv::Point2f> points1;
    std::vector<cv::Point2f> points2;

    std::vector<cv::Point2f> points1o;
    std::vector<cv::Point2f> points2o;

    for (size_t i = 0; i < lines.size(); i++)
    {
        float rho = lines[i][0], theta = lines[i][1];
        Point2f pt1, pt2;
        double a = cos(theta), b = sin(theta);
        double x0 = a * rho, y0 = b * rho;
        pt1.x = cvRound(x0 + 2000 * (-b));
        pt1.y = cvRound(y0 + 2000 * (a));
        pt2.x = cvRound(x0 - 2000 * (-b));
        pt2.y = cvRound(y0 - 2000 * (a));
        if (y0 > 0)
        {
            points1.push_back(pt1);
            points2.push_back(pt2);
        }
        else
        {
            points1o.push_back(pt1);
            points2o.push_back(pt2);
        }
    }
    // clean up to average line
    cv::Point2f sum1 = std::accumulate(points1.begin(), points1.end(), cv::Point2f(0.0f, 0.0f), std::plus<cv::Point2f>());
    pt1 = sum1 / float(points1.size());

    cv::Point2f sum2 = std::accumulate(points2.begin(), points2.end(), cv::Point2f(0.0f, 0.0f), std::plus<cv::Point2f>());
    pt2 = sum2 / float(points2.size());

    cv::Point2f sum3 = std::accumulate(points1o.begin(), points1o.end(), cv::Point2f(0.0f, 0.0f), std::plus<cv::Point2f>());
    pt3 = sum3 / float(points1o.size());

    cv::Point2f sum4 = std::accumulate(points2o.begin(), points2o.end(), cv::Point2f(0.0f, 0.0f), std::plus<cv::Point2f>());
    pt4 = sum4 / float(points2o.size());

    line(input, pt1, pt2, Scalar(0, 0, 255), 3, LINE_AA);
    line(input, pt3, pt4, Scalar(0, 0, 255), 3, LINE_AA);

    // namedWindow("regions", WINDOW_NORMAL);
    // imshow("regions", input);

    return input;
}

Mat process_ball(Mat input, Mat &output, RotatedRect &minRect)
{

    vector<vector<Point>> contours;
    vector<Vec4i> hierarchy;
    Mat ball;
    Mat fullImageHSV;
    cvtColor(input, fullImageHSV, COLOR_BGR2HSV);
    // namedWindow("HSV", WINDOW_NORMAL);
    // imshow("HSV", fullImageHSV);
    inRange(fullImageHSV, Scalar(0, 50, 0), Scalar(100, 255, 255), ball);
    findContours(ball, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE, Point(0, 0));
    vector<double> areas;
    Mat drawing = Mat::zeros(ball.size(), CV_8UC3);
    erode(ball, ball, getStructuringElement(MORPH_RECT, Size(3, 3), Point(1, 1)));
    dilate(ball, ball, getStructuringElement(MORPH_RECT, Size(3, 3), Point(1, 1)));

    float maxArea = 0;
    int maxAreaIndex = 0;

    for (int i = 0; i < contours.size(); i++)
    {
        double area = contourArea(contours[i]);
        if (area > 2500 && area > maxArea)
        {
            maxArea = area;
            maxAreaIndex = i;
        }
    }
    if (contours.size() > 0 && maxArea > 0)
    {
        minRect = minAreaRect(contours[maxAreaIndex]);
        Point2f rect_points[4];
        minRect.points(rect_points);
        Scalar color = Scalar(0, 255, 0);
        for (int j = 0; j < 4; j++)
            line(drawing, rect_points[j], rect_points[(j + 1) % 4], color, 1, 8);
        drawContours(drawing, contours, maxAreaIndex, color, 2, 8, hierarchy, 0, Point());
    }
    // namedWindow("drawing", WINDOW_NORMAL);
    // imshow("drawing", drawing);
    return drawing;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    { // Expect 3 arguments: the program name, the source video and the destination video
        std::cerr << "\nUsage: " << argv[1] << "Input Video Path:" << argv[2] << "Input Video Path:" << std::endl;
        return 1;
    }
    cout << argv[1] << endl;
    cout << argv[2] << endl;

    VideoCapture cap(argv[1]);
    if (!cap.isOpened())
    {
        cout << "Error opening video stream or file" << endl;
        return -1;
    }
    VideoWriter outputVideo;

    Size S = Size((int)cap.get(CAP_PROP_FRAME_WIDTH),
                  (int)cap.get(CAP_PROP_FRAME_HEIGHT));
    int ex = static_cast<int>(cap.get(CAP_PROP_FOURCC));
    outputVideo.open(argv[2], ex, cap.get(CAP_PROP_FPS), S, true);
    if (!outputVideo.isOpened())
    {
        cout << "Could not open the output video for write:" << endl;
        return -1;
    }
    auto count = 0;
    while (cap.isOpened())
    {
        count += 1;
        Mat frame;
        cap >> frame;
        if (frame.empty())
            break;

        // resize(frame, frame, Size(frame.cols / 2.0, frame.rows / 2.0)); 
        Mat output, output2;
        RotatedRect minRect;
        Mat ball = process_ball(frame, output, minRect);
        Point2f pt1, pt2, pt3, pt4;
        find_regions(frame, output2, pt1, pt2, pt3, pt4);

        if (minRect.center.x != 0 && minRect.center.y != 0)
        {
            auto line1 = directionOfPoint(Point(pt1), Point(pt2), minRect.center);
            auto line2 = directionOfPoint(Point(pt3), Point(pt4), minRect.center);
            Point2f rect_points[4];
            minRect.points(rect_points);
            for (int j = 0; j < 4; j++)
                line(frame, rect_points[j], rect_points[(j + 1) % 4], Scalar(0, 255, 0), 1, 8);
            auto zone = "0";
            if (line1 > 0 && line2 > 0)
            {
                zone = "4";
            }
            if (line1 < 0 && line2 > 0)
            {
                zone = "1";
            }
            if (line1 < 0 && line2 < 0)
            {
                zone = "3";
            }
            if (line1 > 0 && line2 < 0)
            {
                zone = "2";
            }

            cv::putText(frame, zone, minRect.center, cv::FONT_HERSHEY_DUPLEX, 1, cv::Scalar(255, 0, 0), 2, false);
            circle(frame, minRect.center, 5, Scalar(0, 0, 255), FILLED, LINE_8);
        }
        line(frame, pt1, pt2, Scalar(0, 0, 255), 3, LINE_AA);
        line(frame, pt3, pt4, Scalar(0, 0, 255), 3, LINE_AA);
        outputVideo << frame;
        // namedWindow("Frame", WINDOW_NORMAL);
        // imshow("Frame", frame);
        // waitKey(0);
    }

    return 0;
}
