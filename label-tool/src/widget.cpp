#include "widget.h"
#include "ui_widget.h"
#include <iostream>
#include <QImage>
#include <QMouseEvent>
#include <QPixmap>
#include <QPoint>
#include <rtabmap/core/CameraRGBD.h>
#include <rtabmap/core/Odometry.h>
#include <rtabmap/core/Optimizer.h>
#include <rtabmap/core/Parameters.h>
#include <rtabmap/core/RtabmapThread.h>
#include <rtabmap/core/SensorData.h>
#include <rtabmap/core/util3d_transforms.h>
#include <rtabmap/core/util3d.h>
#include <rtabmap/utilite/UEventsManager.h>
#include <string>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    // setup DBDriver
    dbDriver = rtabmap::DBDriver::create();

    // connect signals and slots
    connect(ui->slider, SIGNAL (valueChanged(int)), this, SLOT (setSliderValue(int)));
    connect(ui->pushButton, SIGNAL (released()), this, SLOT (saveLabel()));
}

Widget::~Widget()
{
    delete ui;
}

void Widget::setDbPath(char *name)
{
    dbPath = QString::fromUtf8(name);
}

bool Widget::openDatabase()
{
    std::string path = dbPath.toStdString();
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK)
    {
        UERROR("Could not open database");
        return false;
    }
    if (!dbDriver->openConnection(path))
    {
        UERROR("Could not open database");
        return false;
    }
    createLabelTable();

    if (!memory.init(path, false))
    {
        UERROR("Error init memory");
        return false;
    }

    return true;
}

void Widget::createLabelTable()
{
    std::string query;
    query = "CREATE TABLE IF NOT EXISTS Labels (\n\t" \
        "labelName VARCHAR(255),\n\t" \
        "imgId INT,\n\t" \
        "x INT,\n\t" \
        "y INT\n); ";
    int rc = sqlite3_exec(db, query.c_str(), NULL, NULL, NULL);
    if (rc != SQLITE_OK)
    {
        UWARN("Could not create label table");
    }
}

bool Widget::setSliderRange()
{
    // get number of images in database
    std::set<int> ids;
    dbDriver->getAllNodeIds(ids);
    numImages = ids.size();

    if (numImages <= 0)
    {
        return false;
    }

    // image ID is 1-indexed
    ui->slider->setRange(1, numImages);

    // display first image
    showImage(1);

    return true;
}

void Widget::setSliderValue(int value)
{
    ui->label_id->setText(QString::number(value));
    showImage(value);
}

void Widget::saveLabel()
{
    std::string label_name = ui->lineEdit_label->text().toStdString();
    std::string label_id = ui->label_id->text().toStdString();
    std::string label_x = ui->label_x->text().toStdString();
    std::string label_y = ui->label_y->text().toStdString();

    if (label_name.length() == 0)
    {
        UWARN("Label name empty");
        return;
    }

    int imageId, x, y;
    imageId = std::stoi(label_id);
    x = std::stoi(label_x);
    y = std::stoi(label_y);

    // convert again to verify label has depth
    if (convertTo3D(imageId, x, y))
    {
        std::stringstream saveQuery;
        saveQuery << "INSERT INTO Labels VALUES ('" \
                    << label_name << "', '" << label_id << "', '" << label_x << "', '" << label_y << "');";

        std::cout << saveQuery.str() << std::endl;
        int rc = sqlite3_exec(db, saveQuery.str().c_str(), NULL, NULL, NULL);
        if (rc != SQLITE_OK)
        {
            UWARN("Could not save label to label table");
        }
    }
    else
    {
        UWARN("Could not convert label");
    }
}

void Widget::showImage(int index)
{
    rtabmap::SensorData data;
    dbDriver->getNodeData(index, data);
    data.uncompressData();
    cv::Mat raw = data.imageRaw();

    QImage image = QImage(raw.data, raw.cols, raw.rows, raw.step, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(image.rgbSwapped()); // need to change BGR -> RGBz

    ui->label_img->setPixmap(pixmap);
}

void Widget::mousePressEvent(QMouseEvent *event)
{
    const QPoint p = event->pos();
    ui->label_x->setText(QString::number(p.x()));
    ui->label_y->setText(QString::number(p.y()));

    convertTo3D(ui->slider->value(), p.x(), p.y());
}

bool Widget::convertTo3D(int imageId, int x, int y)
{
    std::map<int, rtabmap::Transform> optimizedPoses = optimizeGraph(memory);
    pcl::PointXYZ pWorld;
    if (convert(imageId, x, y, memory, optimizedPoses, pWorld))
    {
        ui->label_status->setText("3D conversion success!");
        return true;
    }
    else
    {
        ui->label_status->setText("Failed to convert");
    }
    return false;
}

/* convert() and optimizeGraph() taken from label_tool/src/main.cpp */
bool Widget::convert(int imageId, int x, int y, rtabmap::Memory &memory, std::map<int, rtabmap::Transform> &poses, pcl::PointXYZ &pWorld)
{
    const rtabmap::SensorData &data = memory.getNodeData(imageId, true);
    const rtabmap::CameraModel &cm = data.cameraModels()[0];
    bool smoothing = false;

    pcl::PointXYZ pLocal = rtabmap::util3d::projectDepthTo3D(data.depthRaw(), x, y, cm.cx(), cm.cy(), cm.fx(), cm.fy(), smoothing);
    if (std::isnan(pLocal.x) || std::isnan(pLocal.y) || std::isnan(pLocal.z))
    {
        UWARN("Depth value not valid");
        return false;
    }
    std::map<int, rtabmap::Transform>::const_iterator iter = poses.find(imageId);
    if (iter == poses.end() || iter->second.isNull())
    {
        UWARN("Image pose not found or is Null");
        return false;
    }
    rtabmap::Transform poseWorld = iter->second;
    poseWorld = poseWorld * cm.localTransform();
    pWorld = rtabmap::util3d::transformPoint(pLocal, poseWorld);
    return true;
}

std::map<int, rtabmap::Transform> Widget::optimizeGraph(rtabmap::Memory &memory)
{
    if (memory.getLastWorkingSignature())
    {
        // Get all IDs linked to last signature (including those in Long-Term Memory)
        std::map<int, int> ids = memory.getNeighborsId(memory.getLastWorkingSignature()->id(), 0);

        UINFO("Optimize poses, ids.size() = %d", ids.size());

        // Get all metric constraints (the graph)
        std::map<int, rtabmap::Transform> poses;
        std::multimap<int, rtabmap::Link> links;
        memory.getMetricConstraints(uKeysSet(ids), poses, links);

        // Optimize the graph
        rtabmap::Optimizer::Type optimizerType = rtabmap::Optimizer::kTypeTORO; // options: kTypeTORO, kTypeG2O, kTypeGTSAM, kTypeCVSBA
        rtabmap::Optimizer *graphOptimizer = rtabmap::Optimizer::create(optimizerType);
        std::map<int, rtabmap::Transform> optimizedPoses = graphOptimizer->optimize(poses.begin()->first, poses, links);
        delete graphOptimizer;

        return optimizedPoses;
    }
}

void Widget::setLabel(const QString &name)
{
    ui->lineEdit_label->setText(name);
}

QString Widget::getLabel() const
{
    return ui->lineEdit_label->text();
}
