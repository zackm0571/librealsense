// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#include <librealsense/rs.hpp>
#include "example.hpp"
#include <librealsense/rs.h>
#include <librealsense/rsutil.h>
#include <thread>
#include <iostream>
#include <algorithm>
using namespace std;
std::vector<texture_buffer> buffers;

void beepFreq(const char* freq){
    
    //system("say -v Daniel \"beep\"");
    //system("echo -e \\\a");
    char cmd[120];
    strcat(cmd, "/usr/local/bin/play -n synth 0.1 sin ");
    strcat(cmd, freq);
    system(cmd);
    
}

int main(int argc, char * argv[]) try
{
    thread alert;
    rs::log_to_console(rs::log_severity::warn);
    //rs::log_to_file(rs::log_severity::debug, "librealsense.log");
    
    rs::context ctx;
    if(ctx.get_device_count() == 0) throw std::runtime_error("No device detected. Is it plugged in?");
    
    // Enumerate all devices
    std::vector<rs::device *> devices;
    for(int i=0; i<ctx.get_device_count(); ++i)
    {
        devices.push_back(ctx.get_device(i));
    }
    
    // Configure and start our devices
    int perTextureWidth = 0;
    int perTextureHeight = 0;
    
    for(auto dev : devices)
    {
        std::cout << "Starting " << dev->get_name() << "... ";
        dev->enable_stream(rs::stream::depth, rs::preset::best_quality);
        dev->start();
        std::cout << "done." << std::endl;
        perTextureWidth = dev->get_stream_width(rs::stream::depth);
        perTextureHeight = dev->get_stream_height(rs::stream::depth);
        }
        // Depth and color
        buffers.resize(ctx.get_device_count());
        
        // Open a GLFW window
        glfwInit();
        std::ostringstream ss; ss << "CPP Multi-Camera Example";
        GLFWwindow * win = glfwCreateWindow(perTextureWidth, perTextureHeight, ss.str().c_str(), 0, 0);
        glfwMakeContextCurrent(win);
        
        int windowWidth, windowHeight;
        glfwGetWindowSize(win, &windowWidth, &windowHeight);
        const float CLOSEST_MULTIPLIER = perTextureHeight,
        IMMEDIATE_MULTIPLIER =  perTextureHeight / 5,
        ARM_LENGTH_MULTIPLIER = perTextureHeight / 4,
        FOOT_STEPS_AHEAD_MULTIPLIER = perTextureHeight / 3,
        WAYS_OFF_MULTIPLIER = perTextureHeight / 2;
        
        while (!glfwWindowShouldClose(win))
        {
            // Wait for new images
            glfwPollEvents();
            
            // Draw the images
            int w,h;
            glfwGetFramebufferSize(win, &w, &h);
            glViewport(0, 0, w, h);
            glClear(GL_COLOR_BUFFER_BIT);
            
            glfwGetWindowSize(win, &w, &h);
            glPushMatrix();
            glOrtho(0, w, h, 0, -1, +1);
            glPixelZoom(1, -1);
            float closest_object = 1000.0f;
            for(auto dev : devices)
            {
                dev->poll_for_frames();
                
                buffers[0].show(*dev, rs::stream::depth, 0, 0, (int)perTextureWidth, (int)perTextureHeight);
                
                const float scale = rs_get_device_depth_scale((const rs_device*)dev, NULL);
                const uint16_t * image = (const uint16_t *)rs_get_frame_data((const rs_device*)dev, RS_STREAM_DEPTH, NULL);
                
                for (int x =  perTextureHeight / 2;
                     x < perTextureWidth + perTextureHeight / 2; x++){
                    float depth_in_meters = scale * image[x];
                    
                    if(depth_in_meters > 0){
                        if(depth_in_meters < closest_object){
                            closest_object = depth_in_meters;
                        }
                        std::printf("%.6f \n ", depth_in_meters);
                        std::printf("%d \n \n \n", x);
                        
                        glBegin(GL_POINTS);
                        glVertex3f(x % perTextureWidth, perTextureHeight / 2, 0);
                        glEnd( );
                        
                    }
                }
                int draw_y = 0;
                for (int _y = perTextureWidth / 2; _y < ((perTextureHeight + perTextureHeight) * perTextureWidth / 2) ; _y += perTextureWidth){
                    float depth_in_meters = scale * image[_y];
                    if(depth_in_meters > 0){
                        if(depth_in_meters < closest_object){
                            closest_object = depth_in_meters;
                        }
                        
                        std::printf("%.6f \n ", depth_in_meters);
                        std::printf("%d \n \n \n", _y);
                        
                        glBegin(GL_POINTS);
                        glVertex3f(perTextureWidth / 2, draw_y, 0);
                        glEnd( );
                    }
                    draw_y++;
                }
            }
            float object_relativity_meter = 0;
            if(closest_object > 0.5 && closest_object < 1){
                object_relativity_meter = CLOSEST_MULTIPLIER;
                alert = thread(beepFreq, "700");
            }
            else if(closest_object > 1 && closest_object < 2){
                object_relativity_meter =  closest_object * IMMEDIATE_MULTIPLIER;
                alert = thread(beepFreq, "500");
            }
            else if(closest_object > 2 && closest_object < 3){
                object_relativity_meter =  closest_object * ARM_LENGTH_MULTIPLIER;
                alert = thread(beepFreq, "400");
            }
            else if(closest_object > 4 && closest_object < 5){
                object_relativity_meter = closest_object * FOOT_STEPS_AHEAD_MULTIPLIER;
                alert = thread(beepFreq, "300");
            }
            else if(closest_object >= 5){
                object_relativity_meter = closest_object * FOOT_STEPS_AHEAD_MULTIPLIER;
                alert = thread(beepFreq, "200");
            }
            else{
                object_relativity_meter = 5.0;
                alert = thread(beepFreq, "200");
            }
          
            glBegin(GL_LINES);
            glVertex3f(5.00f, perTextureHeight - 5.00f, 0.0f); // origin of the FIRST line
            glVertex3f(5.00f, closest_object * 100.00f, 0.0f); // ending point of the FIRST line
            glVertex3f(6.00f, perTextureHeight - 5.00f, 0.0f); // origin of the SECOND line
            glVertex3f(6.00f, closest_object * 10000.00f, 0.0f); // ending point of the SECOND line
            glVertex3f(7.00f, perTextureHeight - 5.00f, 0.0f); // origin of the SECOND line
            glVertex3f(7.00f, closest_object * 10000.00f, 0.0f); // ending point of the SECOND line
            glEnd( );
            glPopMatrix();
            glfwSwapBuffers(win);
            
            alert.join();
        }
        
        glfwDestroyWindow(win);
        glfwTerminate();
        return EXIT_SUCCESS;
        }
        catch(const rs::error & e)
        {
            std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        catch(const std::exception & e)
        {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }