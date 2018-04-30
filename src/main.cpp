#include <iostream>

#include <igl/opengl/glfw/Viewer.h>
#include <igl/readOFF.h>
#include <igl/opengl/glfw/imgui/ImGuiMenu.h>
#include <igl/boundary_loop.h>
#include <igl/harmonic.h>
#include <igl/map_vertices_to_circle.h>
#include <igl/jet.h>
#include <igl/gaussian_curvature.h>
#include <Eigen/Core>

using namespace Eigen;
using namespace std;

MatrixXd V(0, 3);                //vertex array, #V x3
MatrixXi F(0, 3);                //face array, #F x3
MatrixXd V_uv(0, 2);             //vertex array in the UV plane, #V x2

VectorXd area_map;               //area map, #F x1
VectorXd gaus_curv_map;          //gaussian curvature map, #V x1
VectorXd control_map;            //control map, #F x1

int num_of_samples;       //number of samples

void harmonic_parameterization();
void calc_area_map();
void calc_gaussian_curvature_map();
void calc_control_map();
void sampling();

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cout << "Usage: remesh_bin mesh.off" << std::endl;
        exit(0);
    }

    // Read mesh
    igl::readOFF(argv[1], V, F);
    assert(V.rows() > 0);

    // Initialize variables
    num_of_samples = V.rows();

    // Plot the mesh
    igl::opengl::glfw::Viewer viewer;
    viewer.data().clear();
    viewer.data().set_mesh(V, F);

    // Setup the menu
    igl::opengl::glfw::imgui::ImGuiMenu menu;
    viewer.plugins.push_back(&menu);
    // Add content to the default menu window
    menu.callback_draw_viewer_menu = [&]()
    {
        if (ImGui::Button("Reset Mesh"))
        {
            viewer.data().clear();

            viewer.data().set_mesh(V, F);
            viewer.core.align_camera_center(V, F);
        }

        if (ImGui::CollapsingHeader("Parameterization", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button("Harmonic"))
            {
                harmonic_parameterization();

                viewer.data().set_mesh(V_uv, F);
                viewer.data().set_uv(V_uv);
                viewer.core.align_camera_center(V_uv, F);

                // Recompute the normal in 2D plane
                viewer.data().compute_normals();
            }
        }

        if (ImGui::CollapsingHeader("Geometry Maps", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::Button("Area Map"))
            {
                calc_area_map();

                MatrixXd color;
                igl::jet(area_map, false, color);

                viewer.data().set_mesh(V_uv, F);
                viewer.data().set_colors(color);
            }

            if (ImGui::Button("Gaussian Curvature Map"))
            {
                calc_gaussian_curvature_map();

                MatrixXd color;
                igl::jet(gaus_curv_map, false, color);

                viewer.data().set_mesh(V_uv, F);
                viewer.data().set_colors(color);
            }

            if (ImGui::Button("Control Map"))
            {
                calc_control_map();

                MatrixXd color;
                igl::jet(control_map, false, color);

                viewer.data().set_mesh(V_uv, F);
                viewer.data().set_colors(color);
            }
        }

        if (ImGui::CollapsingHeader("Sampling", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::InputInt("samples", &num_of_samples);

            if (ImGui::Button("Perform Sampling"))
            {
                sampling();
            }
        }
    };
    
    viewer.launch();
}

void harmonic_parameterization()
{
    // Find the open boundary
    VectorXi bnd;
    igl::boundary_loop(F, bnd);

    // Map the boundary to a circle, preserving edge proportions
    MatrixXd bnd_uv;
    igl::map_vertices_to_circle(V, bnd, bnd_uv);

    // Harmonic parametrization for the internal vertices
    igl::harmonic(V, F, bnd, bnd_uv, 1, V_uv);
}

void calc_area_map()
{
    ArrayXd dblA3D;
    ArrayXd dblA2D;

    igl::doublearea(V, F, dblA3D);
    igl::doublearea(V_uv, F, dblA2D);

    area_map.resize(F.rows());
    area_map << dblA3D / dblA2D;
}

void calc_gaussian_curvature_map()
{
    // Calculate per-vertex discrete gaussian curvature
    VectorXd K;
    igl::gaussian_curvature(V, F, K);

    // Using the mean per-vertex gaussian curvature to estimate the curvature on each face
    gaus_curv_map.resize(F.rows());
    for (int i = 0; i < F.rows(); i++)
    {
        gaus_curv_map(i) = (K(F(i, 0)) + K(F(i, 1)) + K(F(i, 2))) / 3.0;
    }
}

void calc_control_map()
{
    control_map.resize(F.rows());
    control_map << area_map.array() * gaus_curv_map.array();
}

void sampling()
{
    
}
