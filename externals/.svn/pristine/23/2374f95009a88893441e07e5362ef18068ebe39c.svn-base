// **************************************************************************
/** @class Interpolator2D

@brief Extra- and interpolate in 2D

This class implements a kind of Delaunay triangulation. It calculated the
Voronoi points and the corresponding Delaunay triangles. Within each
triangle a bi-linear interpolation is provided.

A special selection criterion is applied for points outside the grid,
so that extrapolation is possible. Note that extrapolation of far away
points (as in the 1D case) is not recommended.

*/
// **************************************************************************
#ifndef FACT_Interpolator2D
#define FACT_Interpolator2D

#include <float.h>
#include <math.h>
#include <vector>
#include <fstream>

class Interpolator2D
{
public:
    struct vec
    {
        double x;
        double y;

        vec(double _x=0, double _y=0) : x(_x), y(_y) { }

        vec orto() const { return vec(-y, x); }

        double dist(const vec &v) const { return hypot(x-v.x, y-v.y); }
        double operator^(const vec &v) const { return x*v.y - y*v.x; }
        vec operator-(const vec &v) const { return vec(x-v.x, y-v.y); }
        vec operator+(const vec &v) const { return vec(x+v.x, y+v.y); }
        vec operator/(double b)     const { return vec(x/b, y/b); }
    };


    struct point : vec
    {
        unsigned int i;
        point(unsigned int _i, const vec &_v) : vec(_v), i(_i) { }
        point(unsigned int _i=0, double _x=0, double _y=0) : vec(_x, _y), i(_i) { }
    };

    struct circle : point
    {
        point p[3];
        double r;

        static bool sameSide(const vec &p1, const vec &p2, const vec &a, const vec &b)
        {
            return ((b-a)^(p1-a))*((b-a)^(p2-a)) > 0;
        }

        bool isInsideTriangle(const vec &v) const
        {
            return sameSide(v, p[0], p[1], p[2]) && sameSide(v, p[1], p[0], p[2]) && sameSide(v, p[2], p[0], p[1]);
        }

        bool isInsideCircle(const vec &v) const
        {
            return dist(v) < r;
        }
    };

    struct weight : point
    {
        circle c;
        double w[3];
    };

private:
    std::vector<point>  inputGrid;   /// positions of the data points (e.g. sensors)
    std::vector<point>  outputGrid;  /// positions at which inter-/extrapolated values should be provided
    std::vector<circle> circles;     /// the calculated circles/triangles
    std::vector<weight> weights;     /// the weights used for the interpolation

    // --------------------------------------------------------------------------
    //
    //! Calculate the collection of circles/triangles which describe the
    //! input grid. This is the collection of circles which are calculated
    //! from any three points and do not contain any other point of the grid.
    //
    void CalculateGrid()
    {
        circles.reserve(2*inputGrid.size());

        // Loop over all triplets of points
        for (auto it0=inputGrid.cbegin(); it0<inputGrid.cend(); it0++)
        {
            for (auto it1=inputGrid.cbegin(); it1<it0; it1++)
            {
                for (auto it2=inputGrid.cbegin(); it2<it1; it2++)
                {
                    // Calculate the circle through the three points

                    // Vectors along the side of the corresponding triangle
                    const vec v1 = *it1 - *it0;
                    const vec v2 = *it2 - *it1;

                    // Orthogonal vectors on the sides
                    const vec n1 = v1.orto();
                    const vec n2 = v2.orto();

                    // Center point of two of the three sides
                    const vec p1 = (*it0 + *it1)/2;
                    const vec p2 = (*it1 + *it2)/2;

                    // Calculate the crossing point of the two
                    // orthogonal vectors originating in the
                    // center of the sides.
                    const double denom = n1^n2;
                    if (denom==0)
                        continue;

                    const vec x(n1.x, n2.x);
                    const vec y(n1.y, n2.y);

                    const vec w(p1^(p1+n1), p2^(p2+n2));

                    circle c;

                    // This is the x and y coordinate of the circle
                    // through the three points and the circle's radius.
                    c.x = (x^w)/denom;
                    c.y = (y^w)/denom;
                    c.r = c.dist(*it1);

                    // Check if any other grid point lays within this circle
                    auto it3 = inputGrid.cbegin();
                    for (; it3<inputGrid.cend(); it3++)
                    {
                        if (it3==it0 || it3==it1 || it3==it2)
                            continue;

                        if (c.isInsideCircle(*it3))
                            break;
                    }

                    // If a point was found inside, reject the circle
                    if (it3!=inputGrid.cend())
                        continue;

                    // Store the three points of the triangle
                    c.p[0] = *it0;
                    c.p[1] = *it1;
                    c.p[2] = *it2;

                    // Keep in list
                    circles.push_back(c);
                }
            }
        }
    }

    // --------------------------------------------------------------------------
    //
    //! Calculate the weights corresponding to the points in the output grid.
    //! Weights are calculated by bi-linear interpolation. For interpolation,
    //! the triangle which contains the point and has the smallest radius
    //! is searched. If this is not available in case of extrapolation,
    //! the condition is relaxed and requires only the circle to contain
    //! the point. If such circle is not available, the circle with the
    //! closest center is chosen.
    //
    bool CalculateWeights()
    {
        weights.reserve(outputGrid.size());

        // Loop over all points in the output grid
        for (auto ip=outputGrid.cbegin(); ip<outputGrid.cend(); ip++)
        {
            double mindd = DBL_MAX;

            auto mint = circles.cend();
            auto minc = circles.cend();
            auto mind = circles.cend();

            for (auto ic=circles.cbegin(); ic<circles.cend(); ic++)
            {
                // Check if point is inside the triangle
                if (ic->isInsideTriangle(*ip))
                {
                    if (mint==circles.cend() || ic->r<mint->r)
                        mint = ic;
                }

                // If we have found such a triangle, no need to check for more
                if (mint!=circles.cend())
                    continue;

                // maybe at least inside the circle
                const double dd = ic->dist(*ip);
                if (dd<ic->r)
                {
                    if (minc==circles.cend() || ic->r<minc->r)
                        minc = ic;
                }

                // If we found such a circle, no need to check for more
                if (minc!=circles.cend())
                    continue;

                // then look for the closest circle center
                if (dd<mindd)
                {
                    mindd = dd;
                    mind  = ic;
                }
            }

            // Choose the best of the three options
            const auto it = mint==circles.cend() ? (minc==circles.cend() ? mind : minc) : mint;
            if (it==circles.cend())
                return false;

            // Calculate the bi-linear interpolation
            const vec &p1 = it->p[0];
            const vec &p2 = it->p[1];
            const vec &p3 = it->p[2];

            const double dy23 = p2.y - p3.y;
            const double dy31 = p3.y - p1.y;
            const double dy12 = p1.y - p2.y;

            const double dx32 = p3.x - p2.x;
            const double dx13 = p1.x - p3.x;
            const double dx21 = p2.x - p1.x;

            const double dxy23 = p2^p3;
            const double dxy31 = p3^p1;
            const double dxy12 = p1^p2;

            const double det = dxy12 + dxy23 + dxy31;

            const double w1 = (dy23*ip->x + dx32*ip->y + dxy23)/det;
            const double w2 = (dy31*ip->x + dx13*ip->y + dxy31)/det;
            const double w3 = (dy12*ip->x + dx21*ip->y + dxy12)/det;

            // Store the original grid-point, the circle's parameters
            // and the calculate weights
            weight w;
            w.x = ip->x;
            w.y = ip->y;
            w.c = *it;
            w.w[0] = w1;
            w.w[1] = w2;
            w.w[2] = w3;

            weights.push_back(w);
        }

        return true;
    }

public:
    // --------------------------------------------------------------------------
    //
    //! Default constructor. Does nothing.
    //
    Interpolator2D()
    {
    }

    // --------------------------------------------------------------------------
    //
    //! Initialize the input grid (the points at which values are known).
    //!
    //! @param n
    //!    number of data points
    //!
    //! @param x
    //!    x coordinates of data points
    //!
    //! @param n
    //!    y coordinates of data points
    //
    Interpolator2D(int n, double *x, double *y)
    {
        SetInputGrid(n, x, y);
    }

    Interpolator2D(const std::vector<Interpolator2D::vec> &v)
    {
        SetInputGrid(v);
    }

    const std::vector<Interpolator2D::weight> getWeights() const { return weights; }
    const std::vector<Interpolator2D::point>  getInputGrid() const { return inputGrid; }
    const std::vector<Interpolator2D::point>  getOutputGrid() const { return outputGrid; }

    // --------------------------------------------------------------------------
    //
    //! helper function to read a grid from a simple file
    //! (alternating x, y)
    //!
    //! @param filename
    //!    filename of ascii file with data
    //!
    //! @returns
    //!    a vector of point with the x and y values.
    //!    in case of failure the vector is empty
    //
    static std::vector<Interpolator2D::vec> ReadGrid(const std::string &filename)
    {
        std::vector<Interpolator2D::vec> grid;

        std::ifstream fin(filename);
        if (!fin.is_open())
            return std::vector<Interpolator2D::vec>();

        while (1)
        {
            double x, y;
            fin >> x;
            fin >> y;
            if (!fin)
                break;

            grid.emplace_back(x, y);
        }

        return fin.bad() ? std::vector<Interpolator2D::vec>() : grid;
    }

    // --------------------------------------------------------------------------
    //
    //! Set a new input grid (the points at which values are known).
    //! Invalidates the output grid and the calculated weights.
    //! Calculates the triangles corresponding to the new grid.
    //!
    //! @param n
    //!    number of data points
    //!
    //! @param x
    //!    x coordinates of data points
    //!
    //! @param n
    //!    y coordinates of data points
    //
    void SetInputGrid(unsigned int n, double *x, double *y)
    {
        circles.clear();
        weights.clear();
        outputGrid.clear();

        inputGrid.clear();
        inputGrid.reserve(n);
        for (unsigned int i=0; i<n; i++)
            inputGrid.emplace_back(i, x[i], y[i]);

        CalculateGrid();
    }

    void SetInputGrid(const std::vector<Interpolator2D::vec> &v)
    {
        circles.clear();
        weights.clear();
        outputGrid.clear();

        inputGrid.clear();
        inputGrid.reserve(v.size());
        for (unsigned int i=0; i<v.size(); i++)
            inputGrid.emplace_back(i, v[i]);

        CalculateGrid();
    }

    /*
    void SetInputGrid(const std::vector<Interpolator2D::point> &v)
    {
        circles.clear();
        weights.clear();
        outputGrid.clear();

        inputGrid.clear();
        inputGrid.reserve(v.size());
        for (unsigned int i=0; i<v.size(); i++)
            inputGrid.emplace_back(v[i], i);

        CalculateGrid();
    }*/

    bool ReadInputGrid(const std::string &filename)
    {
        const auto grid = ReadGrid(filename);
        if (grid.empty())
            return false;

        SetInputGrid(grid);
        return true;
    }


    // --------------------------------------------------------------------------
    //
    //! Set a new output grid (the points at which you want interpolated
    //! or extrapolated values). Calculates new weights.
    //!
    //! @param n
    //!    number of points
    //!
    //! @param x
    //!    x coordinates of points
    //!
    //! @param n
    //!    y coordinates of points
    //!
    //! @returns
    //!    false if the calculation of the weights failed, true in
    //!    case of success
    //
    bool SetOutputGrid(std::size_t n, double *x, double *y)
    {
        if (inputGrid.empty() && n==0)
            return false;

        weights.clear();

        outputGrid.clear();
        outputGrid.reserve(n);
        for (std::size_t i=0; i<n; i++)
            outputGrid.emplace_back(i, x[i], y[i]);

        return CalculateWeights();
    }

    /*
    bool SetOutputGrid(const std::vector<std::pair<double,double>> &v)
    {
        if (inputGrid.empty() || v.empty())
            return false;

        weights.clear();

        outputGrid.clear();
        outputGrid.reserve(v.size());
        for (std::size_t i=0; i<v.size(); i++)
            outputGrid.emplace_back(i, v[i].first, v[i].second);

        return CalculateWeights();
    }*/

    bool SetOutputGrid(const std::vector<Interpolator2D::vec> &v)
    {
        if (inputGrid.empty())
            return false;

        weights.clear();

        outputGrid.clear();
        outputGrid.reserve(v.size());
        for (std::size_t i=0; i<v.size(); i++)
            outputGrid.emplace_back(i, v[i]);

        return CalculateWeights();
    }

    bool ReadOutputGrid(const std::string &filename)
    {
        const auto grid = ReadGrid(filename);
        if (grid.empty())
            return false;

        return SetOutputGrid(grid);
    }


    // --------------------------------------------------------------------------
    //
    //! Perform interpolation. 
    //!
    //! @param z
    //!    Values at the coordinates of the input grid. The order
    //!    must be identical.
    //!
    //! @returns
    //!    A vector<double> is returned with the interpolated values in the
    //!    same order than the putput grid. If the provided vector does
    //!    not match the size of the inputGrid, an empty vector is returned.
    //
    std::vector<double> Interpolate(const std::vector<double> &z) const
    {
        if (z.size()!=inputGrid.size())
            return std::vector<double>();

        std::vector<double> rc;
        rc.reserve(z.size());

        for (auto it=weights.cbegin(); it<weights.cend(); it++)
            rc.push_back(z[it->c.p[0].i] * it->w[0] + z[it->c.p[1].i] * it->w[1] + z[it->c.p[2].i] * it->w[2]);

        return rc;
    }
};
#endif
