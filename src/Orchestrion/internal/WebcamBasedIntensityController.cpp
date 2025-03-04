/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2024 Matthieu Hodgkinson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "WebcamBasedIntensityController.h"
#include <opencv2/videoio.hpp>

#include <iostream>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>
#include 
#include <stdio.h>

using namespace cv;
using namespace std;

// https://learnopencv.com/head-pose-estimation-using-opencv-and-dlib/

namespace dgk
{
void WebcamBasedIntensityController::init()
{
  cv::Mat im = cv::imread("C:/Users/saint/Downloads/headPose.webp");

  // 2D image points. If you change the image, you need to change vector
  std::vector<cv::Point2d> image_points;
  image_points.push_back(cv::Point2d(359, 391)); // Nose tip
  image_points.push_back(cv::Point2d(399, 561)); // Chin
  image_points.push_back(cv::Point2d(337, 297)); // Left eye left corner
  image_points.push_back(cv::Point2d(513, 301)); // Right eye right corner
  image_points.push_back(cv::Point2d(345, 465)); // Left Mouth corner
  image_points.push_back(cv::Point2d(453, 469)); // Right mouth corner

  // 3D model points.
  const std::vector<cv::Point3d> model_points{
      cv::Point3d(0.f, 0.f, 0.f),          // Nose tip
      cv::Point3d(0.f, -330.f, -65.f),     // Chin
      cv::Point3d(-225.f, 170.f, -135.f),  // Left eye left corner
      cv::Point3d(225.f, 170.f, -135.f),   // Right eye right corner
      cv::Point3d(-150.f, -150.f, -125.f), // Left Mouth corner
      cv::Point3d(150.f, -150.f, -125.f)}; // Right mouth corner

  // Camera internals
  // double focal_length = im.cols; // Approximate focal length.
  // Point2d center = cv::Point2d(im.cols / 2, im.rows / 2);
  // cv::Mat camera_matrix = (cv::Mat_<double>(3, 3) << focal_length, 0,
  // center.x,
  //                          0, focal_length, center.y, 0, 0, 1);
  // cv::Mat dist_coeffs = cv::Mat::zeros(
  //     4, 1, cv::DataType<double>::type); // Assuming no lens distortion

  // cout << "Camera Matrix " << endl << camera_matrix << endl;
  // // Output rotation and translation
  // cv::Mat rotation_vector; // Rotation in axis-angle form
  // cv::Mat translation_vector;

  // // Solve for pose
  // cv::solvePnP(model_points, image_points, camera_matrix, dist_coeffs,
  //              rotation_vector, translation_vector);

  // // Project a 3D point (0, 0, 1000.0) onto the image plane.
  // // We use this to draw a line sticking out of the nose

  // vector<Point3d> nose_end_point3D;
  // vector<Point2d> nose_end_point2D;
  // nose_end_point3D.push_back(Point3d(0, 0, 1000.0));

  // projectPoints(nose_end_point3D, rotation_vector, translation_vector,
  //               camera_matrix, dist_coeffs, nose_end_point2D);

  // for (int i = 0; i < image_points.size(); i++)
  // {
  //   circle(im, image_points[i], 3, Scalar(0, 0, 255), -1);
  // }

  // cv::line(im, image_points[0], nose_end_point2D[0], cv::Scalar(255, 0, 0),
  // 2);

  // cout << "Rotation Vector " << endl << rotation_vector << endl;
  // cout << "Translation Vector" << endl << translation_vector << endl;

  // cout << nose_end_point2D << endl;

  // // Display image.
  // cv::imshow("Output", im);
  // cv::waitKey(0);

  // Webcam stream

  Mat frame;
  //--- INITIALIZE VIDEOCAPTURE
  VideoCapture cap;
  // open the default camera using default API
  // cap.open(0);
  // OR advance usage: select any API backend
  int deviceID = 0;        // 0 = open default camera
  int apiID = cv::CAP_ANY; // 0 = autodetect default API
  // open selected camera using selected API
  cap.open(deviceID, apiID);
  // check if we succeeded
  if (!cap.isOpened())
  {
    cerr << "ERROR! Unable to open camera\n";
    return;
  }
  //--- GRAB AND WRITE LOOP
  cout << "Start grabbing" << endl << "Press any key to terminate" << endl;
  for (;;)
  {
    // wait for a new frame from camera and store it into 'frame'
    cap.read(frame);
    // check if we succeeded
    if (frame.empty())
    {
      cerr << "ERROR! blank frame grabbed\n";
      break;
    }

    double focal_length = im.cols; // Approximate focal length.
    Point2d center = cv::Point2d(im.cols / 2, im.rows / 2);
    cv::Mat camera_matrix = (cv::Mat_<double>(3, 3) << focal_length, 0,
                             center.x, 0, focal_length, center.y, 0, 0, 1);
    cv::Mat dist_coeffs = cv::Mat::zeros(
        4, 1, cv::DataType<double>::type); // Assuming no lens distortion
    cv::Mat rotation_vector;               // Rotation in axis-angle form
    cv::Mat translation_vector;
    cv::solvePnP(model_points, image_points, camera_matrix, dist_coeffs,
                 rotation_vector, translation_vector);

    vector<Point3d> nose_end_point3D;
    vector<Point2d> nose_end_point2D;
    nose_end_point3D.push_back(Point3d(0, 0, 1000.0));
    projectPoints(nose_end_point3D, rotation_vector, translation_vector,
                  camera_matrix, dist_coeffs, nose_end_point2D);

    for (int i = 0; i < image_points.size(); i++)
    {
      circle(im, image_points[i], 3, Scalar(0, 0, 255), -1);
    }

    cv::line(im, image_points[0], nose_end_point2D[0], cv::Scalar(255, 0, 0),
             2);

    // show live and wait for a key with timeout long enough to show images
    imshow("Live", im);
    if (waitKey(5) >= 0)
      break;
  }
}

muse::async::Channel<float> WebcamBasedIntensityController::NewIntensity() const
{
  return m_newIntensity;
}

} // namespace dgk