#include "stage/HTTPServer.h"
#include "data/CameraModel.h"
#include "event/DetectionEvent.h"
#include "event/FailureEvent.h"
#include "event/QueryEvent.h"
#include "stage/FeatureExtraction.h"
#include "util/Time.h"
#include <QCoreApplication>
#include <cstdlib>
#include <string.h>
#include <strings.h>

const std::string HTTPServer::busypage =
    "This server is busy, please try again later.";
const std::string HTTPServer::errorpage = "This doesn't seem to be right.";

HTTPServer::HTTPServer()
    : _daemon(nullptr), _numClients(0), _feature(nullptr) {}

HTTPServer::~HTTPServer() {
  stop();
  _numClients = 0;
  _feature = nullptr;
}

bool HTTPServer::start(uint16_t port, unsigned int maxClients) {
  _maxClients = maxClients;

  // start MHD daemon, listening on port
  unsigned int flags = MHD_USE_SELECT_INTERNALLY | MHD_USE_EPOLL_LINUX_ONLY;
  _daemon = MHD_start_daemon(flags, port, nullptr, nullptr, &answerConnection,
                             static_cast<void *>(this),
                             MHD_OPTION_NOTIFY_COMPLETED, &requestCompleted,
                             static_cast<void *>(this), MHD_OPTION_END);
  if (_daemon == nullptr) {
    return false;
  }

  return true;
}

void HTTPServer::stop() {
  if (_daemon != nullptr) {
    MHD_stop_daemon(_daemon);
    _daemon = nullptr;
  }
}

int HTTPServer::getMaxClients() const { return _maxClients; }

int HTTPServer::getNumClients() const { return _numClients; }

void HTTPServer::setNumClients(int numClients) { _numClients = numClients; }

void HTTPServer::setFeatureExtraction(FeatureExtraction *feature) {
  _feature = feature;
}

bool HTTPServer::event(QEvent *event) {
  if (event->type() == DetectionEvent::type()) {
    DetectionEvent *detectionEvent = static_cast<DetectionEvent *>(event);
    ConnectionInfo *connInfo = const_cast<ConnectionInfo *>(
        static_cast<const ConnectionInfo *>(detectionEvent->getSession()));
    connInfo->names = detectionEvent->takeNames();
    connInfo->Session = detectionEvent->takeSession();
    connInfo->detected.release();
    return true;
  } else if (event->type() == FailureEvent::type()) {
    FailureEvent *failureEvent = static_cast<FailureEvent *>(event);
    ConnectionInfo *connInfo = const_cast<ConnectionInfo *>(
        static_cast<const ConnectionInfo *>(failureEvent->getSession()));
    connInfo->detected.release();
    return true;
  }
  return QObject::event(event);
}

int HTTPServer::answerConnection(void *cls, struct MHD_Connection *connection,
                                 const char *url, const char *method,
                                 const char *version, const char *upload_data,
                                 size_t *upload_data_size, void **con_cls) {
  if (strcasecmp(method, MHD_HTTP_METHOD_POST) != 0) {
    return sendPage(connection, errorpage, MHD_HTTP_BAD_REQUEST);
  }

  HTTPServer *httpServer = static_cast<HTTPServer *>(cls);

  if (*con_cls == nullptr) // new connection
  {
    if (httpServer->getNumClients() >= httpServer->getMaxClients()) {
      return sendPage(connection, busypage, MHD_HTTP_SERVICE_UNAVAILABLE);
    }

    ConnectionInfo *connInfo = new ConnectionInfo();

    connInfo->Session.reset(new Session());

    // reserve enough space for an image
    connInfo->rawData.reset(new std::vector<char>());
    connInfo->rawData->reserve(IMAGE_INIT_SIZE);

    connInfo->postProcessor =
        MHD_create_post_processor(connection, POST_BUFFER_SIZE, iteratePost,
                                  static_cast<void *>(connInfo));
    if (connInfo->postProcessor == nullptr) {
      return MHD_NO;
    }

    httpServer->setNumClients(httpServer->getNumClients() + 1);

    connInfo->sessionType = POST;

    *con_cls = static_cast<void *>(connInfo);

    return MHD_YES;
  }

  ConnectionInfo *connInfo = static_cast<ConnectionInfo *>(*con_cls);

  if (*upload_data_size != 0) {
    MHD_post_process(connInfo->postProcessor, upload_data, *upload_data_size);
    *upload_data_size = 0;

    return MHD_YES;
  } else {
    if (!connInfo->rawData->empty()) {
      // all data are received
      connInfo->Session->overallStart = getTime(); // log start of processing

      double fx = connInfo->cameraInfo.fx;
      double fy = connInfo->cameraInfo.fy;
      double cx = connInfo->cameraInfo.cx;
      double cy = connInfo->cameraInfo.cy;
      std::unique_ptr<cv::Mat> image(new cv::Mat());
      std::unique_ptr<CameraModel> camera(new CameraModel());
      createData(*(connInfo->rawData), fx, fy, cx, cy, *image, *camera);
      if (image->empty()) {
        // TODO do I need to free anything here?
        return sendPage(connection, errorpage, MHD_HTTP_BAD_REQUEST);
      }

      QCoreApplication::postEvent(
          httpServer->_feature,
          new QueryEvent(std::move(image), std::move(camera),
                         std::move(connInfo->Session), connInfo));
    }

    // wait for the result to come
    connInfo->detected.acquire();

    std::string answer = "None";
    if (connInfo->names != nullptr && !connInfo->names->empty()) {
      answer = std::move(connInfo->names->at(0));
    }

    return sendPage(connection, answer, MHD_HTTP_OK);
  }

  return sendPage(connection, errorpage, MHD_HTTP_BAD_REQUEST);
}

int HTTPServer::iteratePost(void *coninfo_cls, enum MHD_ValueKind kind,
                            const char *key, const char *filename,
                            const char *content_type,
                            const char *transfer_encoding, const char *data,
                            uint64_t off, size_t size) {
  ConnectionInfo *connInfo = static_cast<ConnectionInfo *>(coninfo_cls);

  if (strcmp(key, "file") != 0 && strcmp(key, "fx") != 0 &&
      strcmp(key, "fy") != 0 && strcmp(key, "cx") != 0 &&
      strcmp(key, "cy") != 0) {
    return MHD_NO;
  }

  if (size > 0) {
    if (strcmp(key, "file") == 0) {
      connInfo->rawData->insert(connInfo->rawData->end(), data, data + size);
    } else if (strcmp(key, "fx") == 0) {
      char *buf = new char[size + 1];
      memcpy(buf, data, size);
      buf[size] = 0;
      connInfo->cameraInfo.fx = atof(buf);
      delete[] buf;
    } else if (strcmp(key, "fy") == 0) {
      char *buf = new char[size + 1];
      memcpy(buf, data, size);
      buf[size] = 0;
      connInfo->cameraInfo.fy = atof(buf);
      delete[] buf;
    } else if (strcmp(key, "cx") == 0) {
      char *buf = new char[size + 1];
      memcpy(buf, data, size);
      buf[size] = 0;
      connInfo->cameraInfo.cx = atof(buf);
      delete[] buf;
    } else if (strcmp(key, "cy") == 0) {
      char *buf = new char[size + 1];
      memcpy(buf, data, size);
      buf[size] = 0;
      connInfo->cameraInfo.cy = atof(buf);
      delete[] buf;
    }
  }

  return MHD_YES;
}

void HTTPServer::requestCompleted(void *cls, struct MHD_Connection *connection,
                                  void **con_cls,
                                  enum MHD_RequestTerminationCode toe) {
  HTTPServer *httpServer = static_cast<HTTPServer *>(cls);
  ConnectionInfo *connInfo = static_cast<ConnectionInfo *>(*con_cls);

  if (connInfo == nullptr) {
    return;
  }

  std::unique_ptr<Session> Session = std::move(connInfo->Session);
  if (Session != nullptr) {
    Session->overallEnd = getTime(); // log processing end time

    std::cout << "TAG_TIME overall "
              << Session->overallEnd - Session->overallStart << " ms"
              << std::endl;
    std::cout << "TAG_TIME features "
              << Session->featuresEnd - Session->featuresStart << " ms"
              << std::endl;
    std::cout << "TAG_TIME words " << Session->wordsEnd - Session->wordsStart
              << " ms" << std::endl;
    std::cout << "TAG_TIME signatures "
              << Session->signaturesEnd - Session->signaturesStart << " ms"
              << std::endl;
    std::cout << "TAG_TIME perspective "
              << Session->perspectiveEnd - Session->perspectiveStart << " ms"
              << std::endl;
  }

  if (connInfo->postProcessor != nullptr) {
    MHD_destroy_post_processor(connInfo->postProcessor);
    httpServer->setNumClients(httpServer->getNumClients() - 1);
  }

  delete connInfo;
  connInfo = nullptr;
  *con_cls = nullptr;
}

int HTTPServer::sendPage(struct MHD_Connection *connection,
                         const std::string &page, int status_code) {
  struct MHD_Response *response = MHD_create_response_from_buffer(
      page.length(),
      const_cast<void *>(static_cast<const void *>(page.c_str())),
      MHD_RESPMEM_PERSISTENT);
  if (!response) {
    return MHD_NO;
  }

  int ret = MHD_queue_response(connection, status_code, response);
  MHD_destroy_response(response);

  return ret;
}

void HTTPServer::createData(const std::vector<char> &data, double fx, double fy,
                            double cx, double cy, cv::Mat &image,
                            CameraModel &camera) {
  // no data copy is needed because conn info
  const bool copyData = false;
  image = imdecode(cv::Mat(data, copyData), cv::IMREAD_GRAYSCALE);

  int width = image.cols;
  int height = image.rows;
  camera = CameraModel("", fx, fy, cx, cy, cv::Size(width, height));
}
