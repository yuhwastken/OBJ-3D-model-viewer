//Thu vien VTK
#include <vtkSmartPointer.h>
#include <vtkAxesActor.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkCaptionActor2D.h>
#include <vtkTextProperty.h>
#include <vtkOBJReader.h>
#include <vtkPNGReader.h>
#include <vtkTexture.h>
#include <vtkPlane.h>
#include <vtkClipPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkInteractorStyleTrackballCamera.h>

//Thu vien serial
#include "serial_input/serialib.h"

//
#include <stdlib.h>
#include <iostream>
#include <math.h>

#define PI 3.1415926535

// Cong ket noi mac dinh cua Arduino
#if defined (_WIN32) || defined(_WIN64)
    #define SERIAL_PORT "\\\\.\\COM1"
#endif
#if defined (__linux__) || defined(__APPLE__)
    #define SERIAL_PORT "/dev/ttyUSB0"
#endif

//Khai bao bien "serial" co kieu du lieu serialib
serialib serial;

//3 goc MPU do duoc
struct TransRot {
    float roll = 0;
    float pitch = 0;
    float yaw = 0;
    float tof = 0;
};

//
bool String_To_TransRot(const char st[], TransRot &transrot) {
    // chuyen string thanh transrot
    int i = 0;
    int acc_idx = 0;
    float acc[4] = {0, 0, 0, 0};
    int temp_idx = 0;
    char temp[32];

    while (st[i] != '\0' && st[i] != '\n' && st[i] != '\r') {
        if (st[i] == ',') {
            temp[temp_idx] = '\0';
            if (acc_idx < 4) {
                acc[acc_idx++] = atof(temp);
            }
            temp_idx = 0;
        } else {
            if (temp_idx < 31) {
                temp[temp_idx++] = st[i];
            }
        }
        i++;
    }

    // Parse the final value after the last comma
    if (temp_idx > 0 && acc_idx < 4) {
        temp[temp_idx] = '\0';
        acc[acc_idx++] = atof(temp);
    }

    // chuyen tu do sang radian
    if (acc_idx == 4) {
        transrot.roll = acc[0]*PI/180;
        transrot.pitch = acc[1]*PI/180;
        transrot.yaw = acc[2]*PI/180;
        transrot.tof = (acc[3] > 600 || acc[3] == -1) ? 0 : acc[3]; // gioi han khoang cach tof nhan duoc
        return true;
    }

    return false;
}

class vtkTimerCallback : public vtkCommand {
public:
    static vtkTimerCallback* New() {
        return new vtkTimerCallback;
    }

    // Pointers to the objects we want to modify
    vtkSmartPointer<vtkPlane> Plane;
    vtkSmartPointer<vtkRenderWindow> RenderWindow;


    float filter_height = -1000.00 , alpha = 0.5 , offset = -2 , tilt_cos;
    float cur_height=0 , cur_roll=0 , cur_pitch=0 , cur_yaw=0;
    
    // The function that runs every time the timer ticks
    // Read serial input and calculating Translation and Rotation of clipping plane
    virtual void Execute(vtkObject* caller, unsigned long eventId, void* vtkNotUsed(callData)) override {
        if (eventId == vtkCommand::TimerEvent) {

            int ret = serial.readString(raw_data, '\n', 63, 2000);
            float h = 0;
            if (ret > 0) {
                TransRot transrot;
                char raw_data[64];
                if (String_To_TransRot(raw_data, transrot)) {

                    tilt_cos = cos(transrot.pitch)*cos(transrot.roll);
                    tilt_cos = (tilt_cos>0.01)?tilt_cos:0.01;

                    if (filter_height == -1000.00){
                        filter_height = transrot.tof * tilt_cos * 0.02;
                    } else {
                        filter_height += 0.1 * (transrot.tof * tilt_cos * 0.02 - filter_height);
                    }

                    cur_height = filter_height + offset;
                    cur_roll = transrot.roll;
                    cur_pitch = transrot.pitch;
                    cur_yaw = transrot.yaw;

                }

            } else {
                std::cerr << "Serial read timeout or error." << std::endl;
            }

            double o[3];
            o[0] =  sin(cur_pitch);
            o[1] =  cos(cur_roll) * cos(cur_pitch);
            o[2] =  sin(cur_roll) * cos(cur_pitch);
            Plane->SetNormal(o[0],o[1],o[2]);
            Plane->SetOrigin(0.0, cur_height, 0.0);

            RenderWindow->Render();
        }
    }
};

int main() {
    //setup cong serial
    char errorOpening = serial.openDevice(SERIAL_PORT, 115200);

    if (errorOpening != 1) {
        std::cout << "Error opening serial port: %d\n" << errorOpening << std::endl;
        return errorOpening;
    }
    std::cout << "Successful connection to %s\n" << SERIAL_PORT << std::endl;

    serial.flushReceiver();

    
    // load obj mesh
    auto reader = vtkSmartPointer<vtkOBJReader>::New();
    reader->SetFileName("chibi.obj");
    reader->Update();

    // load texture png file
    auto pngReader = vtkSmartPointer<vtkPNGReader>::New();
    pngReader->SetFileName("chibi.png");
    
    //uv unwrap
    auto texture = vtkSmartPointer<vtkTexture>::New();
    texture->SetInputConnection(pngReader->GetOutputPort());
    // texture->InterpolateOn(); if the image was flipped

    // clipping plane + normal vector of the plane
    auto plane = vtkSmartPointer<vtkPlane>::New();
    plane->SetOrigin(0.0, 0.0, 0.0);    // The plane point
    plane->SetNormal(0.0, 1.0, 0.0);    // The up vector

    //vtkClipPolyData clip the triangle and automatically make new triangles
    // vtkClipPolyData clip the polygons, not the texture
    auto clipper = vtkSmartPointer<vtkClipPolyData>::New();
    clipper->SetInputConnection(reader->GetOutputPort());
    clipper->SetClipFunction(plane);
    clipper->Update();

    // sau khi clipping, texture tại các tam giác bị lỗi
    // after clipping texture handling
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(clipper->GetOutputPort());

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->SetTexture(texture);

    auto renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->AddActor(actor);
    renderer->SetBackground(0.1, 0.1, 0.1); // Dark gray background

    auto axes = vtkSmartPointer<vtkAxesActor>::New();
    axes->SetTotalLength(1.0, 1.0, 1.0);
    renderer->AddActor(axes);

    auto renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);
    renderWindow->SetSize(1024, 768);
    renderWindow->SetWindowName("window");

    auto interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor->SetRenderWindow(renderWindow);
    auto style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    interactor->SetInteractorStyle(style);
    interactor->Initialize();

    auto axesWidget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    axesWidget->SetOrientationMarker(axes);
    axesWidget->SetInteractor(interactor);
    axesWidget->SetViewport(0.0, 0.0, 0.15, 0.15);
    axesWidget->SetEnabled(1);
    axesWidget->InteractiveOff();

    auto cb = vtkSmartPointer<vtkTimerCallback>::New();
    cb->Plane = plane;
    cb->RenderWindow = renderWindow;

    // Attach the callback to the interactor's TimerEvent
    interactor->AddObserver(vtkCommand::TimerEvent, cb);
    interactor->CreateRepeatingTimer(33); //callback every 33ms -> 1000ms / 30fps = 33ms

    renderWindow->Render();
    interactor->Start();

    return 0;
}
