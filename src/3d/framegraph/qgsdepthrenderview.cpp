/***************************************************************************
  qgsdepthrenderview.cpp
  --------------------------------------
  Date                 : August 2022
  Copyright            : (C) 2022 by Benoit De Mezzo and (C) 2020 by Belgacem Nedjima
  Email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsdepthrenderview.h"
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QLayerFilter>
#include <Qt3DRender/QLayer>
#include <Qt3DRender/QRenderTargetSelector>
#include <Qt3DRender/QRenderTarget>
#include <Qt3DRender/QTexture>
#include <Qt3DRender/QClearBuffers>
#include <Qt3DRender/qsubtreeenabler.h>
#include <Qt3DRender/QCameraSelector>
#include <Qt3DRender/QRenderStateSet>
#include <Qt3DRender/QDepthTest>
#include <Qt3DRender/QCullFace>
#include <Qt3DRender/QRenderCapture>
#include "qgsdepthentity.h"

QgsDepthRenderView::QgsDepthRenderView( QObject *parent
                                        , Qt3DRender::QCamera *mainCamera
                                        , Qt3DRender::QTexture2D *forwardDepthTexture
                                        , Qt3DCore::QEntity *rootSceneEntity )
  : QgsAbstractRenderView( parent, "depth" )
  , mMainCamera( mainCamera )
{
  mLayer = new Qt3DRender::QLayer;
  mLayer->setRecursive( true );
  mLayer->setObjectName( mViewName + "::Layer" );

  // depth rendering pass
  constructDepthRenderPass( forwardDepthTexture, rootSceneEntity );
}

Qt3DRender::QFrameGraphNode *QgsDepthRenderView::constructDepthRenderPass( Qt3DRender::QTexture2D *forwardDepthTexture
    , Qt3DCore::QEntity *rootSceneEntity )
{
  // depth buffer render to copy pass

  Qt3DRender::QCameraSelector *cameraSelector = new Qt3DRender::QCameraSelector( mRendererEnabler );
  cameraSelector->setObjectName( mViewName + "::CameraSelector" );
  cameraSelector->setCamera( mMainCamera );

  Qt3DRender::QRenderStateSet *stateSet = new Qt3DRender::QRenderStateSet( cameraSelector );

  Qt3DRender::QDepthTest *depthTest = new Qt3DRender::QDepthTest;
  depthTest->setDepthFunction( Qt3DRender::QDepthTest::Always );;
  Qt3DRender::QCullFace *cullFace = new Qt3DRender::QCullFace;
  cullFace->setMode( Qt3DRender::QCullFace::NoCulling );

  stateSet->addRenderState( depthTest );
  stateSet->addRenderState( cullFace );

  Qt3DRender::QLayerFilter *layerFilter = new Qt3DRender::QLayerFilter( stateSet );
  layerFilter->addLayer( mLayer );

  mRenderTargetSelector = new Qt3DRender::QRenderTargetSelector( layerFilter );

  // Note: We do not a clear buffers node since we are drawing a quad that will override the buffer's content anyway
  mDepthRenderCapture = new Qt3DRender::QRenderCapture( mRenderTargetSelector );

  // entity used to draw the depth texture and convert it to rgb image
  new QgsDepthEntity( forwardDepthTexture, mLayer, rootSceneEntity );

  return cameraSelector;
}
