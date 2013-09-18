//==============================================================================
// Includes.
//==============================================================================
#include <reader/PclReader.h>

// User includes.
#include <common.h>

// C++ includes.
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

// C includes.
#include <assert.h>

// PCL includes.
#include <pcl/registration/lum.h>
#include <pcl/registration/correspondence_estimation.h>

//==============================================================================
// Class implementation.
//==============================================================================
// Constructors.
PclReader::PclReader() : PcReader()
{

}

PclReader::PclReader(const PclReader &other) : PcReader(other)
{}

PclReader::~PclReader()
{}

// Public methods.
void PclReader::read(const string &path,
                     const int &start, const int &end, const int &width,
                     const string &root, const string &ext, const string &poseExt)
{
    cout << "Reading scans..." << endl;
    assert(start >= 0);
    assert(start <= end);
    assert(width > 0);

    // Make sure that we first clear all the previously loaded point clouds.
    this->m_PointClouds.clear();

    // Go from start to end and read point clouds.
    for (int it = start; it <= end; ++it)
    {
        string fileRoot = root + int2String(it, width);
        string fullPath = path;

        if (*path.rbegin() != this->fileSep)
        {
            fullPath += this->fileSep;
        }

        fullPath += fileRoot;

        string pcFilePath = fullPath + ext;
        string poseFilePath = fullPath + poseExt;

        cout << "Loading " << pcFilePath << "..." << endl;

        // Open the stream to the pc file.
        ifstream pcFile(pcFilePath.c_str());
        if (pcFile.is_open() == false) {
            die("Failed to open file \"" + pcFilePath + "\"...");
        }

        // Read in the point cloud.
        pcl::PointCloud<pcl::PointXYZ>::Ptr p_Pc(new pcl::PointCloud<pcl::PointXYZ>);

        double x, y, z;
        while (!pcFile.eof())
        {
            pcl::PointXYZ pt;
            pcFile >> pt.x >> pt.y >> pt.z;
            p_Pc->points.push_back(pt);
        }

        // Open the stream to the pose file.
        ifstream poseFile(poseFilePath.c_str());
        if (poseFile.is_open() == false) {
            die("Failed to open file \"" + poseFilePath + "\"...");
        }

        // Read in the pose.
        Pose pose;
        poseFile >> pose.x >> pose.y >> pose.z;
        poseFile >> pose.roll >> pose.pitch >> pose.yaw;

        // Add the point cloud and pose to the list.
        this->m_PointClouds.push_back(p_Pc);
        this->m_Poses.push_back(pose);

        cout << "Loaded point cloud with " << p_Pc->points.size() << " points..."
             << endl << endl;
    }
}

void PclReader::run()
{
    cout << "Running PCL Lu and Milios Scan Matching algorithm..." << endl;
    assert(this->m_PointClouds.size() == this->m_Poses.size());

    pcl::registration::LUM<pcl::PointXYZ> lum;

    // Add SLAM Graph vertices.
    for (int it = 0; it < this->m_PointClouds.size(); ++it)
    {
        Eigen::Vector6f pose;
        pose << this->m_Poses[it].x, this->m_Poses[it].y, this->m_Poses[it].z,
                this->m_Poses[it].roll, this->m_Poses[it].pitch, this->m_Poses[it].yaw;

        lum.addPointCloud(this->m_PointClouds[it], pose);
    }

    // Add the correspondence results as edges to the SLAM graph.
    for (int it = 0; it < this->m_PointClouds.size() - 1; ++it)
    {
        size_t src = it;
        size_t dst = it + 1;

        pcl::registration::CorrespondenceEstimation<pcl::PointXYZ, pcl::PointXYZ> est;
        est.setInputSource(this->m_PointClouds[src]);
        est.setInputTarget(this->m_PointClouds[dst]);

        pcl::CorrespondencesPtr corresp(new pcl::Correspondences);
        est.determineCorrespondences(*corresp);
        lum.setCorrespondences(src, dst, corresp);
    }

    // Close the loop.
    size_t src = this->m_PointClouds.size() - 1;
    size_t dst = 0;

    pcl::registration::CorrespondenceEstimation<pcl::PointXYZ, pcl::PointXYZ> est;
    est.setInputSource(this->m_PointClouds[src]);
    est.setInputTarget(this->m_PointClouds[dst]);

    pcl::CorrespondencesPtr corresp(new pcl::Correspondences);
    est.determineCorrespondences(*corresp);
    lum.setCorrespondences(src, dst, corresp);

    // TODO need to be changed to match the ones from 3DTK.
    // lum.setMaxIterations();
    // lum.setConvergenceThreshold();

    lum.compute();

    for (int it = 0; it < this->m_PointClouds.size(); ++it)
    {
        cout << lum.getPose(it)
             << endl << endl;
    }
}