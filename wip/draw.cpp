#include "tui.h"
#include <cmath>
#include <thread>

struct Vec3 {
    float x, y, z;
    Vec3 operator+(Vec3 other)  const {return {x+other.x,y+other.y,z+other.z};}
    Vec3 operator+(float other) const {return {x+other,y+other,z+other};}
    Vec3 operator-(Vec3 other)  const {return {x-other.x,y-other.y,z-other.z};}
    Vec3 operator-(float other) const {return {x-other,y-other,z-other};}
    Vec3 operator*(Vec3 other)  const {return {x*other.x,y*other.y,z*other.z};}
    Vec3 operator*(float other) const {return {x*other,y*other,z*other};}
    Vec3 operator/(Vec3 other)  const {return {x/other.x,y/other.y,z/other.z};}
    Vec3 operator/(float other) const {return {x/other,y/other,z/other};}
    float mag() const {return sqrt(x*x+y*y+z*z);}
    float sum() const {return x+y+z;}
    float dot(Vec3 other)  const {return ((*this)*other).sum();}
    Vec3 cross(Vec3 other) const {return {
        y*other.z - z*other.y,
        z*other.x - x*other.z,
        x*other.y - y*other.x
    };}
    Vec3 norm(){return (*this)/mag();}
};

struct Ray {
    Vec3 position;
    Vec3 direction;
};

struct RenderConfig {
    float(*distance)(Ray);
    TUI::RGB(*color)(Ray);
    float min_dist;
    size_t step_limit;
};

TUI::RGB march(Ray ray, RenderConfig config) {
    size_t step = 0;
    float dist = config.distance(ray);
    while( (dist > config.min_dist) && (step < config.step_limit) ) { 
        ray.position = ray.position + ray.direction * dist * 0.5;
        dist = config.distance(ray);
        step++;
    }
    if( dist <= config.min_dist) {
        return config.color(ray);
    } else {
        return TUI::RGB{0,0,0};
    }
}

struct Camera {
    Vec3 position;
    Vec3 direction;
    Vec3 frustrum_bounds;

    void render(TUI::Canvas& canvas, RenderConfig config) {
        Vec3 right = direction.cross({0,0,1}).norm();
        Vec3 up    = direction.cross(right).norm();
        size_t height = canvas.get_height();
        size_t width  = canvas.get_width();
        for (size_t y=0; y<height; y++) {
            for (size_t x=0; x<width; x++) {
                float x_offset = (x - width  * 0.5) / width;
                float z_offset = (y - height * 0.5) / height;
                Vec3 dir = direction*frustrum_bounds.y
                         + right    *(frustrum_bounds.x*x_offset)
                         + up       *(frustrum_bounds.z*z_offset);
                dir = dir.norm();
                Ray ray = {position+dir*frustrum_bounds.z,dir};
                canvas(x,y) = march(ray,config);
            }
        }
    }
};

TUI::RGB color(Ray ray) {
    return {
        (uint8_t)(fmod(ray.position.x,1.0)*255),
        (uint8_t)(fmod(ray.position.y,1.0)*255),
        (uint8_t)(fmod(ray.position.z,1.0)*255),
    };
}

float dist(Ray ray) {
    ray.position.x = fmod(ray.position.x,10.0);
    ray.position.y = fmod(ray.position.y,10.0);
    ray.position.z = fmod(ray.position.z,10.0);
    return (ray.position - Vec3{0,5,0}).mag() - 1.0;
}

int main() {
    size_t width  = 64;
    size_t height = 32;
    size_t span = 100;
    TUI::Canvas canvas(width,height);
    Camera cam = {{0,0,0},{0,1,0},{1,1,1}};
    RenderConfig config {
        .distance   = dist,
        .color      = color,
        .min_dist   = 0.1,
        .step_limit = 100,
    };

    for(int i=0; i< 100; i++){
        cam.render(canvas,config);
        canvas.display();
        std::cout.flush();
        cam.position.x += 0.1;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

