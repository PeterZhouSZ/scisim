// StateOutput.cpp
//
// Breannan Smith
// Last updated: 09/10/2015

#include "StateOutput.h"

#include "SCISim/HDF5File.h"
#include "SCISim/Utilities.h"
#include "RigidBody2DGeometry.h"
#include "RigidBody2DStaticPlane.h"
#include "PlanarPortal.h"
#include "CircleGeometry.h"

void RigidBody2DStateOutput::writeGeometryIndices( const std::vector<std::unique_ptr<RigidBody2DGeometry>>& geometry, const VectorXu& indices, const std::string& group, HDF5File& output_file )
{
  #ifdef USE_HDF5
  using HDFSID = HDFID<H5Sclose>;
  using HDFDID = HDFID<H5Dclose>;

  // Map global indices to 'local' indices; that is, given a global index, gives the index into the particular type (e.g. global index 10 could be sphere number 3)
  VectorXu global_local_geo_mapping( geometry.size() );
  {
    unsigned current_geo_idx{ 0 };
    Vector1u geo_type_local_indices{ Vector1u::Zero() };
    for( const std::unique_ptr<RigidBody2DGeometry>& current_geo : geometry )
    {
      assert(current_geo->type() <= geo_type_local_indices.size() - 1 );
      global_local_geo_mapping( current_geo_idx++ ) = geo_type_local_indices( current_geo->type() )++;
    }
  }

  // Create an HDF5 dataspace
  const hsize_t dim[]{ 2, hsize_t( indices.size() ) };
  const HDFSID data_space{ H5Screate_simple( 2, dim, nullptr ) };
  if( data_space < 0 )
  {
    throw std::string{ "Failed to create HDF dataspace for geometry indices" };
  }

  // Create an HDF5 dataset
  const HDFDID data_set{ H5Dcreate2( output_file.fileID(), ( group + "/geometry_indices" ).c_str(), H5T_NATIVE_UINT, data_space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT ) };
  if( data_set < 0 )
  {
    throw std::string{ "Failed to create HDF dataset for geometry indices" };
  }

  // Create an HDF5 memspace to allow us to insert elements one by one
  const HDFSID mem_space{ H5Screate_simple( 1, dim, nullptr ) };
  if( mem_space < 0 )
  {
    throw std::string{ "Failed to create HDF memspace for geometry indices" };
  }

  // Insert the geometry indices one by one
  unsigned current_geo{ 0 };
  for( unsigned idx = 0; idx < indices.size(); ++idx )
  {
    const unsigned geo_idx{ indices(idx) };
    const hsize_t count[]{ 2, 1 };
    const hsize_t offset[]{ 0, current_geo++ };
    const hsize_t mem_offset[]{ 0, 0 };
    H5Sselect_hyperslab( data_space, H5S_SELECT_SET, offset, nullptr, count, nullptr );
    H5Sselect_hyperslab( mem_space, H5S_SELECT_SET, mem_offset, nullptr, count, nullptr );
    const unsigned data[]{ unsigned( geometry[geo_idx]->type() ), global_local_geo_mapping( geo_idx ) };
    if( H5Dwrite( data_set, H5T_NATIVE_UINT, mem_space, data_space, H5P_DEFAULT, &data ) < 0 )
    {
      throw std::string{ "Failed to write geometry indices to HDF" };
    }
  }
  assert( current_geo == indices.size() );
  #else
  throw std::string{ "writeGeometryIndices not compiled with HDF5 support" };
  #endif
}

#include <iostream>
void writeCircleGeometry( const std::vector<std::unique_ptr<RigidBody2DGeometry>>& geometry, const unsigned circle_count, const std::string& group, HDF5File& output_file )
{
  #ifdef USE_HDF5
  using HDFTID = HDFID<H5Tclose>;
  using HDFSID = HDFID<H5Sclose>;
  using HDFDID = HDFID<H5Dclose>;

  struct CircleData
  {
    scalar r;
  };

  // Create an HDF5 dataspace
  const hsize_t dim[]{ circle_count };
  const HDFSID data_space{ H5Screate_simple( 1, dim, nullptr ) };
  if( data_space < 0 )
  {
    throw std::string{ "Failed to create HDF dataspace for circle geometry" };
  }

  // Create an HDF5 struct for the data
  const HDFTID struct_tid{ H5Tcreate( H5T_COMPOUND, sizeof(CircleData) ) };
  if( struct_tid < 0 )
  {
    throw std::string{ "Failed to create HDF struct" };
  }
  // Insert the r type in the struct
  if( H5Tinsert( struct_tid, "r", HOFFSET(CircleData,r), H5T_NATIVE_DOUBLE ) < 0 )
  {
    throw std::string{ "Failed to create HDF r type for circle geometry" };
  }

  // Create an HDF5 dataset
  const HDFDID data_set{ H5Dcreate2( output_file.fileID(), ( group + "/circles" ).c_str(), struct_tid, data_space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT ) };
  if( data_set < 0 )
  {
    throw std::string{ "Failed to create HDF dataset for circle geometry" };
  }

  // Create an HDF5 memspace to allow us to insert elements one by one
  const HDFSID mem_space{ H5Screate_simple( 1, dim, nullptr ) };
  if( mem_space < 0 )
  {
    throw std::string{ "Failed to create HDF memspace for circle geometry" };
  }

  // Insert the spheres one by one
  unsigned current_circle{ 0 };
  CircleData data;
  for( const std::unique_ptr<RigidBody2DGeometry>& geometry_instance : geometry )
  {
    if( geometry_instance->type() == RigidBody2DGeometry::CIRCLE )
    {
      const CircleGeometry& circle{ sd_cast<const CircleGeometry&>( *geometry_instance ) };
      hsize_t count[]{ 1 };
      hsize_t offset[]{ current_circle++ };
      hsize_t mem_offset[]{ 0 };
      H5Sselect_hyperslab( data_space, H5S_SELECT_SET, offset, nullptr, count, nullptr );
      H5Sselect_hyperslab( mem_space, H5S_SELECT_SET, mem_offset, nullptr, count, nullptr );
      data.r = circle.r();
      if( H5Dwrite( data_set, struct_tid, mem_space, data_space, H5P_DEFAULT, &data ) < 0 )
      {
        throw std::string{ "Failed to write circle geometry struct to HDF" };
      }
    }
  }
  assert( current_circle == circle_count );
  #else
  throw std::string{ "writeCircleGeometry not compiled with HDF5 support" };
  #endif
}

void RigidBody2DStateOutput::writeGeometry( const std::vector<std::unique_ptr<RigidBody2DGeometry>>& geometry, const std::string& group, HDF5File& output_file )
{
  Vector1u body_count{ Vector1u::Zero() };
  for( const std::unique_ptr<RigidBody2DGeometry>& geometry_instance : geometry )
  {
    assert( geometry_instance->type() < body_count.size() );
    ++body_count( geometry_instance->type() );
  }
  assert( body_count.sum() == geometry.size() );

  if( body_count( 0 ) != 0 )
  {
    writeCircleGeometry( geometry, body_count( 0 ), group, output_file );
  }
}

// TODO: Abstract out the struct type creation into own routines
void RigidBody2DStateOutput::writeStaticPlanes( const std::vector<RigidBody2DStaticPlane>& static_planes, const std::string& group, HDF5File& output_file )
{
  #ifdef USE_HDF5
  using HDFTID = HDFID<H5Tclose>;
  using HDFSID = HDFID<H5Sclose>;
  using HDFDID = HDFID<H5Dclose>;

  struct LocalStaticPlaneData
  {
    scalar x[2];
    scalar n[2];
    //scalar t[2]; // Tangent can be recovered from n by user
  };

  // Create an HDF5 dataspace
  const hsize_t dim[]{ static_planes.size() };
  const HDFSID data_space{ H5Screate_simple( 1, dim, nullptr ) };
  if( data_space < 0 )
  {
    throw std::string{ "Failed to create HDF dataspace for static planes" };
  }

  // Create an HDF5 struct for the data
  const HDFTID struct_tid{ H5Tcreate( H5T_COMPOUND, sizeof(LocalStaticPlaneData) ) };
  if( struct_tid < 0 )
  {
    throw std::string{ "Failed to create HDF struct for static planes" };
  }
  // Insert the x type in the struct
  {
    const hsize_t array_dim[]{ 2 };
    const HDFTID array_tid{ H5Tarray_create2( H5T_NATIVE_DOUBLE, 1, array_dim ) };
    if( array_tid < 0 )
    {
      throw std::string{ "Failed to create HDF x type for static planes" };
    }
    if( H5Tinsert( struct_tid, "x", HOFFSET(LocalStaticPlaneData,x), array_tid ) < 0 )
    {
      throw std::string{ "Failed to insert x in HDF struct for static planes" };
    }
  }
  // Insert the R type in the struct
  {
    const hsize_t array_dim[]{ 2 };
    const HDFTID array_tid{ H5Tarray_create2( H5T_NATIVE_DOUBLE, 1, array_dim ) };
    if( array_tid < 0 )
    {
      throw std::string{ "Failed to create HDF n type for static planes" };
    }
    if( H5Tinsert( struct_tid, "n", HOFFSET(LocalStaticPlaneData,n), array_tid ) < 0 )
    {
      throw std::string{ "Failed to insert n in HDF struct for static planes" };
    }
  }

  // Create an HDF5 dataset
  const HDFDID data_set{ H5Dcreate2( output_file.fileID(), ( group + "/static_planes" ).c_str(), struct_tid, data_space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT ) };
  if( data_set < 0 )
  {
    throw std::string{ "Failed to create HDF dataset for static planes" };
  }

  // Create an HDF5 memspace to allow us to insert elements one by one
  const HDFSID mem_space{ H5Screate_simple( 1, dim, nullptr ) };
  if( mem_space < 0 )
  {
    throw std::string{ "Failed to create HDF memspace for static planes" };
  }

  // Insert the static planes one by one
  unsigned current_plane{ 0 };
  LocalStaticPlaneData local_data;
  for( const RigidBody2DStaticPlane& plane : static_planes )
  {
    Eigen::Map<Vector2s>{ local_data.x } = plane.x();
    Eigen::Map<Vector2s>{ local_data.n } = plane.n();
    const hsize_t count[]{ 1 };
    const hsize_t offset[]{ current_plane++ };
    const hsize_t mem_offset[]{ 0 };
    H5Sselect_hyperslab( data_space, H5S_SELECT_SET, offset, nullptr, count, nullptr );
    H5Sselect_hyperslab( mem_space, H5S_SELECT_SET, mem_offset, nullptr, count, nullptr );
    if( H5Dwrite( data_set, struct_tid, mem_space, data_space, H5P_DEFAULT, &local_data ) < 0 )
    {
      throw std::string{ "Failed to write static plane struct to HDF" };
    }
  }
  assert( current_plane == static_planes.size() );
  #else
  throw std::string{ "writeStaticPlanes not compiled with HDF5 support" };
  #endif
}

void RigidBody2DStateOutput::writePlanarPortals( const std::vector<PlanarPortal>& planar_portals, const std::string& group, HDF5File& output_file )
{
  #ifdef USE_HDF5
  using HDFTID = HDFID<H5Tclose>;
  using HDFSID = HDFID<H5Sclose>;
  using HDFDID = HDFID<H5Dclose>;

  struct LocalPlanarPortalData
  {
    // Data for the first plane
    scalar x0[2];
    scalar n0[2];
    // Data for the second plane
    scalar x1[2];
    scalar n1[2];
    // Lees-Edwards data
    scalar v0;
    scalar v1;
    scalar bounds0[2];
    scalar bounds1[2];
    scalar delta0;
    scalar delta1;
  };

  // Create an HDF5 dataspace
  const hsize_t dim[]{ planar_portals.size() };
  const HDFSID data_space{ H5Screate_simple( 1, dim, nullptr ) };
  if( data_space < 0 )
  {
    throw std::string{ "Failed to create HDF dataspace for planar portals" };
  }

  // Create an HDF5 struct for the data
  const HDFTID struct_tid{ H5Tcreate( H5T_COMPOUND, sizeof(LocalPlanarPortalData) ) };
  if( struct_tid < 0 )
  {
    throw std::string{ "Failed to create HDF struct for planar portals" };
  }
  // Insert the x0 type in the struct
  {
    const hsize_t array_dim[]{ 2 };
    const HDFTID array_tid{ H5Tarray_create2( H5T_NATIVE_DOUBLE, 1, array_dim ) };
    if( array_tid < 0 )
    {
      throw std::string{ "Failed to create HDF x0 type for planar portals" };
    }
    if( H5Tinsert( struct_tid, "x0", HOFFSET(LocalPlanarPortalData,x0), array_tid ) < 0 )
    {
      throw std::string{ "Failed to insert x0 in HDF struct for planar portals" };
    }
  }
  // Insert the n0 type in the struct
  {
    const hsize_t array_dim[]{ 2 };
    const HDFTID array_tid{ H5Tarray_create2( H5T_NATIVE_DOUBLE, 1, array_dim ) };
    if( array_tid < 0 )
    {
      throw std::string{ "Failed to create HDF n0 type for planar portals" };
    }
    if( H5Tinsert( struct_tid, "n0", HOFFSET(LocalPlanarPortalData,n0), array_tid ) < 0 )
    {
      throw std::string{ "Failed to insert n0 in HDF struct for planar portals" };
    }
  }
  // Insert the x1 type in the struct
  {
    const hsize_t array_dim[]{ 2 };
    const HDFTID array_tid{ H5Tarray_create2( H5T_NATIVE_DOUBLE, 1, array_dim ) };
    if( array_tid < 0 )
    {
      throw std::string{ "Failed to create HDF x1 type for planar portals" };
    }
    if( H5Tinsert( struct_tid, "x1", HOFFSET(LocalPlanarPortalData,x1), array_tid ) < 0 )
    {
      throw std::string{ "Failed to insert x1 in HDF struct for planar portals" };
    }
  }
  // Insert the n1 type in the struct
  {
    const hsize_t array_dim[]{ 2 };
    const HDFTID array_tid{ H5Tarray_create2( H5T_NATIVE_DOUBLE, 1, array_dim ) };
    if( array_tid < 0 )
    {
      throw std::string{ "Failed to create HDF n1 type for planar portals" };
    }
    if( H5Tinsert( struct_tid, "n1", HOFFSET(LocalPlanarPortalData,n1), array_tid ) < 0 )
    {
      throw std::string{ "Failed to insert n1 in HDF struct for planar portals" };
    }
  }
  // Insert the v0 type in the struct
  if( H5Tinsert( struct_tid, "v0", HOFFSET(LocalPlanarPortalData,v0), H5T_NATIVE_DOUBLE ) < 0 )
  {
    throw std::string{ "Failed to create HDF v0 type for planar portals" };
  }
  // Insert the v1 type in the struct
  if( H5Tinsert( struct_tid, "v1", HOFFSET(LocalPlanarPortalData,v1), H5T_NATIVE_DOUBLE ) < 0 )
  {
    throw std::string{ "Failed to create HDF v1 type for planar portals" };
  }
  // Insert the bounds0 type in the struct
  {
    const hsize_t array_dim[]{ 2 };
    const HDFTID array_tid{ H5Tarray_create2( H5T_NATIVE_DOUBLE, 1, array_dim ) };
    if( array_tid < 0 )
    {
      throw std::string{ "Failed to create HDF bounds0 type for planar portals" };
    }
    if( H5Tinsert( struct_tid, "bounds0", HOFFSET(LocalPlanarPortalData,bounds0), array_tid ) < 0 )
    {
      throw std::string{ "Failed to insert bounds0 in HDF struct for planar portals" };
    }
  }
  // Insert the bounds1 type in the struct
  {
    const hsize_t array_dim[]{ 2 };
    const HDFTID array_tid{ H5Tarray_create2( H5T_NATIVE_DOUBLE, 1, array_dim ) };
    if( array_tid < 0 )
    {
      throw std::string{ "Failed to create HDF bounds1 type for planar portals" };
    }
    if( H5Tinsert( struct_tid, "bounds1", HOFFSET(LocalPlanarPortalData,bounds1), array_tid ) < 0 )
    {
      throw std::string{ "Failed to insert bounds1 in HDF struct for planar portals" };
    }
  }
  // Insert the delta0 type in the struct
  if( H5Tinsert( struct_tid, "delta0", HOFFSET(LocalPlanarPortalData,delta0), H5T_NATIVE_DOUBLE ) < 0 )
  {
    throw std::string{ "Failed to create HDF delta0 type for planar portals" };
  }
  // Insert the delta1 type in the struct
  if( H5Tinsert( struct_tid, "delta1", HOFFSET(LocalPlanarPortalData,delta1), H5T_NATIVE_DOUBLE ) < 0 )
  {
    throw std::string{ "Failed to create HDF delta1 type for planar portals" };
  }

  // Create an HDF5 dataset
  const HDFDID data_set{ H5Dcreate2( output_file.fileID(), ( group + "/planar_portals" ).c_str(), struct_tid, data_space, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT ) };
  if( data_set < 0 )
  {
    throw std::string{ "Failed to create HDF dataset for planar portals" };
  }

  // Create an HDF5 memspace to allow us to insert elements one by one
  const HDFSID mem_space{ H5Screate_simple( 1, dim, nullptr ) };
  if( mem_space < 0 )
  {
    throw std::string{ "Failed to create HDF memspace for planar portals" };
  }

  // Insert the planar portals one by one
  unsigned current_portal{ 0 };
  LocalPlanarPortalData local_data;
  for( const PlanarPortal& planar_portal : planar_portals )
  {
    Eigen::Map<Vector2s>{ local_data.x0 } = planar_portal.planeA().x();
    Eigen::Map<Vector2s>{ local_data.n0 } = planar_portal.planeA().n();
    Eigen::Map<Vector2s>{ local_data.x1 } = planar_portal.planeB().x();
    Eigen::Map<Vector2s>{ local_data.n1 } = planar_portal.planeB().n();
    local_data.v0 = planar_portal.vA();
    local_data.v1 = planar_portal.vB();
    Eigen::Map<Vector2s>{ local_data.bounds0 } = planar_portal.boundsA();
    Eigen::Map<Vector2s>{ local_data.bounds1 } = planar_portal.boundsB();
    local_data.delta0 = planar_portal.deltaA();
    local_data.delta1 = planar_portal.deltaB();
    const hsize_t count[]{ 1 };
    const hsize_t offset[]{ current_portal++ };
    const hsize_t mem_offset[]{ 0 };
    H5Sselect_hyperslab( data_space, H5S_SELECT_SET, offset, nullptr, count, nullptr );
    H5Sselect_hyperslab( mem_space, H5S_SELECT_SET, mem_offset, nullptr, count, nullptr );
    if( H5Dwrite( data_set, struct_tid, mem_space, data_space, H5P_DEFAULT, &local_data ) < 0 )
    {
      throw std::string{ "Failed to write planar_portal struct to HDF" };
    }
  }
  assert( current_portal == planar_portals.size() );
  #else
  throw std::string{ "writePlanarPortals not compiled with HDF5 support" };
  #endif
}