/* Copyright (c) 2014, Julian Straub <jstraub@csail.mit.edu>
 * Licensed under the MIT license. See the license file LICENSE.
 */
#include <iostream>
#include <pcl/io/pcd_io.h>
#include <pcl/io/ply_io.h>
#include <pcl/point_types.h>
#include <pcl/visualization/cloud_viewer.h>
#include <pcl/features/integral_image_normal.h>
#include <pcl/features/normal_3d.h>

#include <boost/program_options.hpp>

namespace po = boost::program_options;
using std::cout;
using std::endl;

#include <jsCore/timer.hpp>

int main (int argc, char** argv)
{

  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", "produce help message")
    ("input,i", po::value<string>(),"path to input")
    ("output,o", po::value<string>(),"path to output")
    ("scale,s", po::value<float>(),"scale for normal extraction search radius")
    ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);    

  if (vm.count("help")) {
    cout << desc << "\n";
    return 1;
  }

  float scale = 0.1;
  string inputPath = "./file.ply";
  string outputPath = "./out.ply";
  if(vm.count("input")) inputPath = vm["input"].as<string>();
  if(vm.count("output")) outputPath = vm["output"].as<string>();
  if(vm.count("scale")) scale = vm["scale"].as<float>();

  cout<< " extracting from "<<inputPath<<endl;
  cout<< " scale for normal estimation radius: "<<scale<<endl;
  cout<< " output to "<<outputPath<<endl;

  pcl::PointCloud<pcl::PointXYZ>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PLYReader reader;
  if (reader.read(inputPath, *cloud)) 
    std::cout << "error reading " << inputPath << std::endl;
  else
    std::cout << "loaded pc from " << inputPath << ": " << cloud->width << "x"
      << cloud->height << std::endl;
//  if (pcl::io::loadPLYFile(inputPath, *cloud))
//  {
//    std::cout << "Cloud reading failed." << std::endl;
//    return (-1);
//  }

//  if (pcl::io::loadPCDFile<pcl::PointXYZ> ("test_pcd.pcd", *cloud) == -1) //* load the file
//  {
//    PCL_ERROR ("Couldn't read file test_pcd.pcd \n");
//    return (-1);
//  }

  int w = cloud->width;
  int h = cloud->height;
  cout<<w << "x"<<h<<endl;

//  cloud->points[10+w*10] = pcl::PointXYZ(1.0,2.0,3.0);
//  cout<<cloud->points.data()<<endl;
//  float* data = cloud->points.data()->data;
//  cout<<data[(10+w*10)*3+0]<<" "<<data[(10+w*10)*3+1]<<" "<<data[(10+w*10)*3+2]<<endl;
//  cout<<cloud->points[10+w*10]<<endl;

  // estimate normals on a organized point cloud
  if(false)
  {
    jsc::Timer t0;
    pcl::PointCloud<pcl::Normal>::Ptr normals (new pcl::PointCloud<pcl::Normal>);
    pcl::IntegralImageNormalEstimation<pcl::PointXYZ, pcl::Normal> ne;
    ne.setNormalEstimationMethod (ne.AVERAGE_3D_GRADIENT); // 31ms
    //ne.setNormalEstimationMethod (ne.AVERAGE_DEPTH_CHANGE); // 23ms
    //ne.setNormalEstimationMethod (ne.COVARIANCE_MATRIX); // 47ms
    ne.setMaxDepthChangeFactor(0.02f);
    ne.setNormalSmoothingSize(10.0f);
    ne.setInputCloud(cloud);
    ne.compute(*normals);
    t0.toc();
    cout<<t0<<endl;
  }

  jsc::Timer t0;
  pcl::PointCloud<pcl::Normal>::Ptr normals (new pcl::PointCloud<pcl::Normal>);
  pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> ne;
//  ne.setNormalEstimationMethod (ne.AVERAGE_3D_GRADIENT); // 31ms
  //ne.setNormalEstimationMethod (ne.AVERAGE_DEPTH_CHANGE); // 23ms
  //ne.setNormalEstimationMethod (ne.COVARIANCE_MATRIX); // 47ms
//  ne.setMaxDepthChangeFactor(0.02f);
//  ne.setNormalSmoothingSize(10.0f);
  ne.setInputCloud(cloud);

  // Create an empty kdtree representation, and pass it to the normal estimation object.
  // Its content will be filled inside the object, based on the given input dataset (as no other search surface is given).
  pcl::search::KdTree<pcl::PointXYZ>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZ> ());
  ne.setSearchMethod (tree);
  ne.setRadiusSearch(scale);

  ne.compute(*normals);
  t0.toc();
  cout<<t0<<endl;

  cout<<normals->width<<" "<<normals->height<<endl;

  pcl::PointCloud<pcl::PointXYZRGBNormal> ptNormals(normals->width,
      normals->height);
  for(uint32_t i=0; i< ptNormals.size(); ++i) {
    ptNormals.at(i).x = cloud->points[i].x;
    ptNormals.at(i).y = cloud->points[i].y;
    ptNormals.at(i).z = cloud->points[i].z;
    if (normals->points[i].normal_x == normals->points[i].normal_x
        && normals->points[i].normal_y == normals->points[i].normal_y
        && normals->points[i].normal_z == normals->points[i].normal_z) {
      ptNormals.at(i).normal_x = normals->points[i].normal_x;
      ptNormals.at(i).normal_y = normals->points[i].normal_y;
      ptNormals.at(i).normal_z = normals->points[i].normal_z;
    } else {
      ptNormals.at(i).normal_x = 0;
      ptNormals.at(i).normal_y = 0;
      ptNormals.at(i).normal_z = 1;
    }
  }
  
  pcl::PLYWriter writer;
  writer.write(outputPath, ptNormals, false, false);

  return 0;

  // visualize normals
  pcl::visualization::PCLVisualizer viewer("PCL Viewer");
  viewer.setBackgroundColor (0.3, 0.3, 0.3);
//  pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> single_color (cloud, 200, 10, 10);
//  viewer.addPointCloud(cloud,single_color,"cloud");
  viewer.addPointCloud(cloud,"cloud");
  viewer.addPointCloudNormals<pcl::PointXYZ,pcl::Normal>(cloud, normals,10,0.05,"normals");
 
  while (!viewer.wasStopped ())
  {
    viewer.spinOnce ();
  }

  return 0;
}
