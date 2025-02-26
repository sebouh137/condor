// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright (C) 2024 Sebouh J. Paul

//==========================================================================
//  Implementation of the CONDOR detector
//--------------------------------------------------------------------------
//  Author: Sebouh J. Paul (UCR)
//==========================================================================

#include "DD4hep/DetFactoryHelper.h"
#include <XML/Helper.h>
#include <XML/Layering.h>

using namespace dd4hep;

static Ref_t createDetector(Detector& desc, xml_h handle, SensitiveDetector sens)
{
  xml_det_t     detElem = handle;
  std::string   detName = detElem.nameStr();
  int           detID   = detElem.id();

  xml_dim_t  dim        = detElem.dimensions();
  double     length     = dim.x(); // Size along x-axis
  double     width      = dim.y(); // Size along y-axis
  double     height     = dim.z(); // Size along z-axis

  xml_dim_t  pos        = detElem.position(); // Position in global coordinates
  xml_dim_t rot         = detElem.rotation();

  // Getting information about "blocks" (ie, groups of superpanels)
  const xml::Component &block_xml = detElem.child(
    _Unicode(block)
  );
  const double nrows = dd4hep::getAttrOrDefault<double>(
    block_xml,
    _Unicode(n_rows),
    14
  );
  const double ncols = dd4hep::getAttrOrDefault<double>(
    block_xml,
    _Unicode(n_cols),
    14
  );
  const double block_dim_x = dd4hep::getAttrOrDefault<double>(
    block_xml,
    _Unicode(dim_x),
    8.7*m
  );
  const double block_dim_y = dd4hep::getAttrOrDefault<double>(
    block_xml,
    _Unicode(dim_y),
    8.1*m
  );
  const double block_dim_z = dd4hep::getAttrOrDefault<double>(
    block_xml,
    _Unicode(dim_z),
    2.*cm
  );

  //superpanels:  groups of panels that are next to each other edgewise
  const xml::Component &superpanels_xml = detElem.child(
    _Unicode(superpanels)
  );
  
  const double superpanel_dim_x = dd4hep::getAttrOrDefault<double>(
    superpanels_xml,
    _Unicode(dim_x),
    1.6*m
  );

  const double superpanel_dim_y = dd4hep::getAttrOrDefault<double>(
    superpanels_xml,
    _Unicode(dim_y),
    7.8*m
  );

  const double superpanel_dim_z = dd4hep::getAttrOrDefault<double>(
    superpanels_xml,
    _Unicode(dim_z),
    2.*cm
  );
  
  const double superpanels_per_block = dd4hep::getAttrOrDefault<double>(
    superpanels_xml,
    _Unicode(per_block),
    4
  );
  
  const double superpanel_spacing_x = dd4hep::getAttrOrDefault<double>(
    superpanels_xml,
    _Unicode(spacing_x),
    2.*m
  );
  
  
  Material   air        = desc.material("Air");

  // Defining envelope
  Box envelope(length / 2.0, width / 2.0, height / 2.0);

  // Defining envelope volume
  Volume envelopeVol(detName, envelope, air);
  // Setting envelope attributes
  envelopeVol.setAttributes(
    desc,
    detElem.regionStr(),
    detElem.limitsStr(),
    "InvisibleWithDaughters" //detElem.visStr()
  );

  PlacedVolume pv;

  for(int i=0; i<ncols; i++){
    for(int j=0; j<nrows; j++){
      if(i==1 || i == ncols-2 || j == 1 || j == nrows-2
	 || ((i==0 || i==ncols-1) && j!=0 && j!=nrows-1 && j%3!=2)
	 || ((j==0 || j==nrows-1) && i!=0 && i!=ncols-1 && i%3!=2))
	continue;
	 
      std::string block_name = detName + _toString(i, "_row_%d") + _toString(j, "_col_%d");
      Box block(block_dim_x / 2., block_dim_y / 2., block_dim_z / 2.);
      Volume block_vol(block_name, block, air);

      /*pv = envelopeVol.placeVolume(
        block_vol,
        Transform3D(
          RotationZYX(0, 0, 0),
          Position(
            (-(ncols-1)/2.+i)*block_dim_x,
            (-(nrows-1)/2.+j)*block_dim_y,
	    0
          )
        )
      );*/
      for(int k=0; k<superpanels_per_block; k++){
	Material superpanel_mat = desc.material(superpanels_xml.materialStr());
	std::string superpanel_name = detName + _toString(i, "_row_%d") + _toString(j, "_col_%d") + _toString(k, "_superpanel_%d");
	Box superpanel(superpanel_dim_x / 2., superpanel_dim_y / 2., superpanel_dim_z / 2.);
	Volume superpanel_vol(superpanel_name, superpanel, superpanel_mat);

	pv =  block_vol.placeVolume(
	  superpanel_vol,
	  Transform3D(
	    RotationZYX(0, 0, 0),
	    Position(
	      (-(superpanels_per_block-1)/2.+k)*superpanel_spacing_x,
	      0.,
	      0.
	    )
	  )
	);
	sens.setType("calorimeter");
	superpanel_vol.setSensitiveDetector(sens);
	superpanel_vol.setAttributes(
				      desc,
          superpanels_xml.regionStr(),
          superpanels_xml.limitsStr(),
          superpanels_xml.visStr()
	);
	
	pv.addPhysVolID("superpanel", k);
	pv.addPhysVolID("row", j);
	pv.addPhysVolID("column", i);
	//	std::cout << i << " " << j << " " << k << std::endl;
      }
      
      pv = envelopeVol.placeVolume(
        block_vol,
        Transform3D(
          RotationZYX(0, 0, 0),
          Position(
            (-(ncols-1)/2.+i)*block_dim_x,
            (-(nrows-1)/2.+j)*block_dim_y,
	    0
          )
        )
      );

      block_vol.setAttributes(
	  desc,
          block_xml.regionStr(),
          block_xml.limitsStr(),
          "InvisibleWithDaughters" //block_xml.visStr()
        );
      pv.addPhysVolID("row", j);
      pv.addPhysVolID("column", i);
    }
  }

  DetElement   det(detName, detID);
  Volume motherVol = desc.pickMotherVolume(det);

  // Placing ZDC in world volume
  auto tr = Transform3D(RotationZYX(rot.z(), rot.y(), rot.x()),Position(pos.x(), pos.y(), pos.z()));
  PlacedVolume phv = motherVol.placeVolume(envelopeVol, tr);
  phv.addPhysVolID("system", detID);
  det.setPlacement(phv);

  return det;
}
DECLARE_DETELEMENT(Condor, createDetector)
