#include <iostream>
#include <cstring>

struct Point {
    int x, y;

    Point () : x(), y() {}
    Point (int _x, int _y) : x(_x), y(_y) {}
};

class Shape {
    int vertices;
    Point** points;
public:
    Shape (int _vertices) {
        vertices = _vertices;
        points = new Point*[vertices+1];
    }

    ~Shape () {
        for (int i = 0; i < vertices+1; i++) {
            delete[] points[i];
        }
        delete[] points; 
    }

    void addPoints (Point* pts) {
        for (int i = 0; i <= vertices; i++) {
            points[i] = new Point[5];
            memcpy(points[i], &pts[i%vertices], sizeof(Point));
        }
    }

    double area () {
        int temp = 0;
        for (int i = 0; i < vertices; i++) {
            int lhs = points[i]->x * points[i+1]->y;
            int rhs = (*points[i+1]).x * (*points[i]).y;
            temp += (lhs - rhs);
        }
        double area(abs(temp)/2.0);
        return area;
    }
};

int main () {
    Point tri1(0, 0);
    Point tri2(1, 2);
    Point tri3(2, 0);

    // adding points to tri
    Point triPts[3] = {tri1, tri2, tri3};
    Shape* tri = new Shape(3);
    tri->addPoints(triPts);

    Point quad1(0, 0);
    Point quad2(0, 2);
    Point quad3(2, 2);
    Point quad4(2, 0);

    // adding points to quad
    Point quadPts[4] = {quad1, quad2, quad3, quad4};
    Shape* quad = new Shape(4);
    quad->addPoints(quadPts);

    std::cout << "Tri: " << tri->area() << std::endl;
    std::cout << "Quad: " << quad->area() << std::endl;

    delete tri;
    delete quad;

}
