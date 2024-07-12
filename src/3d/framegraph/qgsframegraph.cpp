/***************************************************************************
  qgsframegraph.cpp
  --------------------------------------
  Date                 : August 2020
  Copyright            : (C) 2020 by Belgacem Nedjima
  Email                : gb underscore nedjima at esi dot dz
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsframegraph.h"
#include "qgsdirectionallightsettings.h"
#include "qgspostprocessingentity.h"
#include "qgspreviewquad.h"
#include "qgs3dutils.h"
#include "qgsfgutils.h"
#include "qgsabstractrenderview.h"

#include "qgsambientocclusionrenderentity.h"
#include "qgsambientocclusionblurentity.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QBuffer>
#include <Qt3DRender/QGeometry>

typedef Qt3DRender::QAttribute Qt3DQAttribute;
typedef Qt3DRender::QBuffer Qt3DQBuffer;
typedef Qt3DRender::QGeometry Qt3DQGeometry;
#else
#include <Qt3DCore/QAttribute>
#include <Qt3DCore/QBuffer>
#include <Qt3DCore/QGeometry>

typedef Qt3DCore::QAttribute Qt3DQAttribute;
typedef Qt3DCore::QBuffer Qt3DQBuffer;
typedef Qt3DCore::QGeometry Qt3DQGeometry;
#endif

#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QTechnique>
#include <Qt3DRender/QGraphicsApiFilter>
#include <Qt3DRender/QBlendEquation>
#include <Qt3DRender/QColorMask>
#include <Qt3DRender/QSortPolicy>
#include <Qt3DRender/QPointSize>
#include <Qt3DRender/QSeamlessCubemap>
#include <Qt3DRender/QNoDepthMask>
#include <Qt3DRender/QBlendEquationArguments>
#include <Qt3DExtras/QTextureMaterial>
#include <Qt3DRender/QAbstractTexture>
#include <Qt3DRender/QNoDraw>
#include "qgsshadowrenderview.h"
#include "qgsforwardrenderview.h"
#include "qgsdepthrenderview.h"
#include "qgsdepthentity.h"

const QString QgsFrameGraph::FORWARD_RENDERVIEW = "forward";
const QString QgsFrameGraph::SHADOW_RENDERVIEW = "shadow";
const QString QgsFrameGraph::AXIS3D_RENDERVIEW = "3daxis";
const QString QgsFrameGraph::DEPTH_RENDERVIEW = "depth";

void QgsFrameGraph::constructForwardRenderPass()
{
  // This is where rendering of the 3D scene actually happens.
  // We define two forward passes: one for solid objects, followed by one for transparent objects.
  //
  //                                  |
  //                         +-----------------+
  //                         | QCameraSelector |  (using the main camera)
  //                         +-----------------+
  //                                  |
  //                         +-----------------+
  //                         |  QLayerFilter   |  (using mForwardRenderLayer)
  //                         +-----------------+
  //                                  |
  //                      +-----------------------+
  //                      | QRenderTargetSelector | (write mForwardColorTexture + mForwardDepthTexture)
  //                      +-----------------------+
  //                                  |
  //         +------------------------+---------------------+
  //         |                                              |
  //  +-----------------+    discard               +-----------------+    accept
  //  |  QLayerFilter   |  transparent             |  QLayerFilter   |  transparent
  //  +-----------------+    objects               +-----------------+    objects
  //         |                                              |
  //  +-----------------+  use depth test          +-----------------+   sort entities
  //  | QRenderStateSet |  cull back faces         |  QSortPolicy    |  back to front
  //  +-----------------+                          +-----------------+
  //         |                                              |
  //  +-----------------+              +--------------------+--------------------+
  //  | QFrustumCulling |              |                                         |
  //  +-----------------+     +-----------------+  use depth tests      +-----------------+  use depth tests
  //         |                | QRenderStateSet |  don't write depths   | QRenderStateSet |  write depths
  //         |                +-----------------+  write colors         +-----------------+  don't write colors
  //  +-----------------+                          use alpha blending                        don't use alpha blending
  //  |  QClearBuffers  |  color and depth         no culling                                no culling
  //  +-----------------+

  Qt3DRender::QTexture2D *forwardColorTexture = new Qt3DRender::QTexture2D;
  forwardColorTexture->setWidth( mSize.width() );
  forwardColorTexture->setHeight( mSize.height() );
  forwardColorTexture->setFormat( Qt3DRender::QAbstractTexture::RGB8_UNorm );
  forwardColorTexture->setGenerateMipMaps( false );
  forwardColorTexture->setMagnificationFilter( Qt3DRender::QTexture2D::Linear );
  forwardColorTexture->setMinificationFilter( Qt3DRender::QTexture2D::Linear );
  forwardColorTexture->wrapMode()->setX( Qt3DRender::QTextureWrapMode::ClampToEdge );
  forwardColorTexture->wrapMode()->setY( Qt3DRender::QTextureWrapMode::ClampToEdge );

  Qt3DRender::QTexture2D *forwardDepthTexture = new Qt3DRender::QTexture2D;
  forwardDepthTexture->setWidth( mSize.width() );
  forwardDepthTexture->setHeight( mSize.height() );
  forwardDepthTexture->setFormat( Qt3DRender::QTexture2D::TextureFormat::DepthFormat );
  forwardDepthTexture->setGenerateMipMaps( false );
  forwardDepthTexture->setMagnificationFilter( Qt3DRender::QTexture2D::Linear );
  forwardDepthTexture->setMinificationFilter( Qt3DRender::QTexture2D::Linear );
  forwardDepthTexture->wrapMode()->setX( Qt3DRender::QTextureWrapMode::ClampToEdge );
  forwardDepthTexture->wrapMode()->setY( Qt3DRender::QTextureWrapMode::ClampToEdge );

  Qt3DRender::QRenderTargetOutput *forwardRenderTargetDepthOutput = new Qt3DRender::QRenderTargetOutput;
  forwardRenderTargetDepthOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Depth );
  forwardRenderTargetDepthOutput->setTexture( forwardDepthTexture );
  forwardRenderTargetDepthOutput->setObjectName( FORWARD_RENDERVIEW + "::RenderTargetDepthOutput" );
  Qt3DRender::QRenderTargetOutput *forwardRenderTargetColorOutput = new Qt3DRender::QRenderTargetOutput;
  forwardRenderTargetColorOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );
  forwardRenderTargetColorOutput->setTexture( forwardColorTexture );
  forwardRenderTargetColorOutput->setObjectName( FORWARD_RENDERVIEW + "::RenderTargetColorOutput" );

  QgsForwardRenderView *forwardRenderView = new QgsForwardRenderView( this, mMainCamera );
  forwardRenderView->setTargetOutputs( { forwardRenderTargetDepthOutput, forwardRenderTargetColorOutput } );
  registerRenderView( forwardRenderView, FORWARD_RENDERVIEW );
}

Qt3DRender::QLayer *QgsFrameGraph::transparentObjectLayer()
{
  QgsForwardRenderView *forwardRenderView = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  if ( forwardRenderView )
    return forwardRenderView->transparentObjectLayer();
  return nullptr;
}

void QgsFrameGraph::constructShadowRenderPass()
{
  Qt3DRender::QTexture2D *shadowMapTexture = new Qt3DRender::QTexture2D;
  shadowMapTexture->setWidth( QgsFrameGraph::mDefaultShadowMapResolution );
  shadowMapTexture->setHeight( QgsFrameGraph::mDefaultShadowMapResolution );
  shadowMapTexture->setFormat( Qt3DRender::QTexture2D::TextureFormat::DepthFormat );
  shadowMapTexture->setGenerateMipMaps( false );
  shadowMapTexture->setMagnificationFilter( Qt3DRender::QTexture2D::Linear );
  shadowMapTexture->setMinificationFilter( Qt3DRender::QTexture2D::Linear );
  shadowMapTexture->wrapMode()->setX( Qt3DRender::QTextureWrapMode::ClampToEdge );
  shadowMapTexture->wrapMode()->setY( Qt3DRender::QTextureWrapMode::ClampToEdge );
  shadowMapTexture->setObjectName( "QgsShadowRenderView::DepthTarget" );

  Qt3DRender::QRenderTargetOutput *renderTargetOutput = new Qt3DRender::QRenderTargetOutput;
  renderTargetOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Depth );
  renderTargetOutput->setTexture( shadowMapTexture );

  QgsShadowRenderView *shadowRenderView = new QgsShadowRenderView( this );
  shadowRenderView->setTargetOutputs( { renderTargetOutput } );
  registerRenderView( shadowRenderView, SHADOW_RENDERVIEW );
}

Qt3DRender::QFrameGraphNode *QgsFrameGraph::constructSubPostPassForTexturesPreview()
{
  Qt3DRender::QLayerFilter *layerFilter = new Qt3DRender::QLayerFilter;
  layerFilter->setObjectName( "Sub pass TexturesPreview" );
  layerFilter->addLayer( mPreviewLayer );

  Qt3DRender::QRenderStateSet *renderStateSet = new Qt3DRender::QRenderStateSet( layerFilter );
  Qt3DRender::QDepthTest *depthTest = new Qt3DRender::QDepthTest;
  depthTest->setDepthFunction( Qt3DRender::QDepthTest::Always );
  renderStateSet->addRenderState( depthTest );
  Qt3DRender::QCullFace *cullFace = new Qt3DRender::QCullFace;
  cullFace->setMode( Qt3DRender::QCullFace::NoCulling );
  renderStateSet->addRenderState( cullFace );

  return layerFilter;
}

Qt3DRender::QFrameGraphNode *QgsFrameGraph::constructSubPostPassForProcessing()
{
  Qt3DRender::QCameraSelector *cameraSelector = new Qt3DRender::QCameraSelector;
  cameraSelector->setObjectName( "Sub pass Postprocessing" );
  cameraSelector->setCamera( dynamic_cast<QgsShadowRenderView *>( renderView( QgsFrameGraph::SHADOW_RENDERVIEW ) )->lightCamera() );

  Qt3DRender::QLayerFilter *layerFilter = new Qt3DRender::QLayerFilter( cameraSelector );

  // could be the first of this branch
  new Qt3DRender::QClearBuffers( layerFilter );

  Qt3DRender::QLayer *postProcessingLayer = new Qt3DRender::QLayer();
  mPostprocessingEntity = new QgsPostprocessingEntity( this, postProcessingLayer, mRootEntity );
  layerFilter->addLayer( postProcessingLayer );
  mPostprocessingEntity->setObjectName( "PostProcessingPassEntity" );

  return cameraSelector;
}

Qt3DRender::QFrameGraphNode *QgsFrameGraph::constructSubPostPassForRenderCapture()
{
  Qt3DRender::QFrameGraphNode *top = new Qt3DRender::QNoDraw;
  top->setObjectName( "Sub pass RenderCapture" );

  mRenderCapture = new Qt3DRender::QRenderCapture( top );

  return top;
}

Qt3DRender::QFrameGraphNode *QgsFrameGraph::constructPostprocessingPass()
{
  mRenderCaptureTargetSelector = new Qt3DRender::QRenderTargetSelector;
  mRenderCaptureTargetSelector->setObjectName( "Postprocessing render pass" );
  mRenderCaptureTargetSelector->setEnabled( mRenderCaptureEnabled );

  Qt3DRender::QRenderTarget *renderTarget = new Qt3DRender::QRenderTarget( mRenderCaptureTargetSelector );

  // The lifetime of the objects created here is managed
  // automatically, as they become children of this object.

  // Create a render target output for rendering color.
  Qt3DRender::QRenderTargetOutput *colorOutput = new Qt3DRender::QRenderTargetOutput( renderTarget );
  colorOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );

  // Create a texture to render into.
  mRenderCaptureColorTexture = new Qt3DRender::QTexture2D( colorOutput );
  mRenderCaptureColorTexture->setSize( mSize.width(), mSize.height() );
  mRenderCaptureColorTexture->setFormat( Qt3DRender::QAbstractTexture::RGB8_UNorm );
  mRenderCaptureColorTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mRenderCaptureColorTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mRenderCaptureColorTexture->setObjectName( "PostProcessingPass::ColorTarget" );

  // Hook the texture up to our output, and the output up to this object.
  colorOutput->setTexture( mRenderCaptureColorTexture );
  renderTarget->addOutput( colorOutput );

  Qt3DRender::QRenderTargetOutput *depthOutput = new Qt3DRender::QRenderTargetOutput( renderTarget );

  depthOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Depth );
  mRenderCaptureDepthTexture = new Qt3DRender::QTexture2D( depthOutput );
  mRenderCaptureDepthTexture->setSize( mSize.width(), mSize.height() );
  mRenderCaptureDepthTexture->setFormat( Qt3DRender::QAbstractTexture::DepthFormat );
  mRenderCaptureDepthTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mRenderCaptureDepthTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mRenderCaptureDepthTexture->setComparisonFunction( Qt3DRender::QAbstractTexture::CompareLessEqual );
  mRenderCaptureDepthTexture->setComparisonMode( Qt3DRender::QAbstractTexture::CompareRefToTexture );
  mRenderCaptureDepthTexture->setObjectName( "PostProcessingPass::DepthTarget" );

  depthOutput->setTexture( mRenderCaptureDepthTexture );
  renderTarget->addOutput( depthOutput );

  mRenderCaptureTargetSelector->setTarget( renderTarget );

  // sub passes:
  constructSubPostPassForProcessing()->setParent( mRenderCaptureTargetSelector );
  constructSubPostPassForTexturesPreview()->setParent( mRenderCaptureTargetSelector );
  constructSubPostPassForRenderCapture()->setParent( mRenderCaptureTargetSelector );

  return mRenderCaptureTargetSelector;
}

Qt3DRender::QFrameGraphNode *QgsFrameGraph::constructAmbientOcclusionRenderPass()
{
  mAmbientOcclusionRenderCameraSelector = new Qt3DRender::QCameraSelector;
  mAmbientOcclusionRenderCameraSelector->setObjectName( "AmbientOcclusion render pass CameraSelector" );
  mAmbientOcclusionRenderCameraSelector->setCamera( mMainCamera );

  mAmbientOcclusionRenderStateSet = new Qt3DRender::QRenderStateSet( mAmbientOcclusionRenderCameraSelector );

  Qt3DRender::QDepthTest *depthRenderDepthTest = new Qt3DRender::QDepthTest;
  depthRenderDepthTest->setDepthFunction( Qt3DRender::QDepthTest::Always );;
  Qt3DRender::QCullFace *depthRenderCullFace = new Qt3DRender::QCullFace;
  depthRenderCullFace->setMode( Qt3DRender::QCullFace::NoCulling );

  mAmbientOcclusionRenderStateSet->addRenderState( depthRenderDepthTest );
  mAmbientOcclusionRenderStateSet->addRenderState( depthRenderCullFace );

  mAmbientOcclusionRenderLayerFilter = new Qt3DRender::QLayerFilter( mAmbientOcclusionRenderStateSet );

  mAmbientOcclusionRenderCaptureTargetSelector = new Qt3DRender::QRenderTargetSelector( mAmbientOcclusionRenderLayerFilter );
  Qt3DRender::QRenderTarget *colorRenderTarget = new Qt3DRender::QRenderTarget( mAmbientOcclusionRenderCaptureTargetSelector );

  // The lifetime of the objects created here is managed
  // automatically, as they become children of this object.

  // Create a render target output for rendering color.
  Qt3DRender::QRenderTargetOutput *colorOutput = new Qt3DRender::QRenderTargetOutput( colorRenderTarget );
  colorOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );

  // Create a texture to render into.
  mAmbientOcclusionRenderTexture = new Qt3DRender::QTexture2D( colorOutput );
  mAmbientOcclusionRenderTexture->setSize( mSize.width(), mSize.height() );
  mAmbientOcclusionRenderTexture->setFormat( Qt3DRender::QAbstractTexture::R32F );
  mAmbientOcclusionRenderTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mAmbientOcclusionRenderTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );

  // Hook the texture up to our output, and the output up to this object.
  colorOutput->setTexture( mAmbientOcclusionRenderTexture );
  colorRenderTarget->addOutput( colorOutput );

  mAmbientOcclusionRenderCaptureTargetSelector->setTarget( colorRenderTarget );

  QgsAbstractRenderView *forwardRenderView = renderView( QgsFrameGraph::FORWARD_RENDERVIEW );
  Qt3DRender::QLayer *ambientOcclusionRenderLayer = new Qt3DRender::QLayer();
  Qt3DRender::QTexture2D *forwardDepthTexture = forwardRenderView->outputTexture( Qt3DRender::QRenderTargetOutput::Depth );
  mAmbientOcclusionRenderEntity = new QgsAmbientOcclusionRenderEntity( forwardDepthTexture, ambientOcclusionRenderLayer, mMainCamera, mRootEntity );
  mAmbientOcclusionRenderLayerFilter->addLayer( ambientOcclusionRenderLayer );

  return mAmbientOcclusionRenderCameraSelector;
}

Qt3DRender::QFrameGraphNode *QgsFrameGraph::constructAmbientOcclusionBlurPass()
{
  mAmbientOcclusionBlurCameraSelector = new Qt3DRender::QCameraSelector;
  mAmbientOcclusionBlurCameraSelector->setObjectName( "AmbientOcclusion blur pass CameraSelector" );
  mAmbientOcclusionBlurCameraSelector->setCamera( mMainCamera );

  mAmbientOcclusionBlurStateSet = new Qt3DRender::QRenderStateSet( mAmbientOcclusionBlurCameraSelector );

  Qt3DRender::QDepthTest *depthRenderDepthTest = new Qt3DRender::QDepthTest;
  depthRenderDepthTest->setDepthFunction( Qt3DRender::QDepthTest::Always );;
  Qt3DRender::QCullFace *depthRenderCullFace = new Qt3DRender::QCullFace;
  depthRenderCullFace->setMode( Qt3DRender::QCullFace::NoCulling );

  mAmbientOcclusionBlurStateSet->addRenderState( depthRenderDepthTest );
  mAmbientOcclusionBlurStateSet->addRenderState( depthRenderCullFace );

  mAmbientOcclusionBlurLayerFilter = new Qt3DRender::QLayerFilter( mAmbientOcclusionBlurStateSet );

  mAmbientOcclusionBlurRenderCaptureTargetSelector = new Qt3DRender::QRenderTargetSelector( mAmbientOcclusionBlurLayerFilter );
  Qt3DRender::QRenderTarget *depthRenderTarget = new Qt3DRender::QRenderTarget( mAmbientOcclusionBlurRenderCaptureTargetSelector );

  // The lifetime of the objects created here is managed
  // automatically, as they become children of this object.

  // Create a render target output for rendering color.
  Qt3DRender::QRenderTargetOutput *colorOutput = new Qt3DRender::QRenderTargetOutput( depthRenderTarget );
  colorOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );

  // Create a texture to render into.
  mAmbientOcclusionBlurTexture = new Qt3DRender::QTexture2D( colorOutput );
  mAmbientOcclusionBlurTexture->setSize( mSize.width(), mSize.height() );
  mAmbientOcclusionBlurTexture->setFormat( Qt3DRender::QAbstractTexture::R32F );
  mAmbientOcclusionBlurTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  mAmbientOcclusionBlurTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );

  // Hook the texture up to our output, and the output up to this object.
  colorOutput->setTexture( mAmbientOcclusionBlurTexture );
  depthRenderTarget->addOutput( colorOutput );

  mAmbientOcclusionBlurRenderCaptureTargetSelector->setTarget( depthRenderTarget );

  Qt3DRender::QLayer *ambientOcclusionBlurLayer = new Qt3DRender::QLayer();
  mAmbientOcclusionBlurEntity = new QgsAmbientOcclusionBlurEntity( mAmbientOcclusionRenderTexture, ambientOcclusionBlurLayer, mRootEntity );
  mAmbientOcclusionBlurLayerFilter->addLayer( ambientOcclusionBlurLayer );

  return mAmbientOcclusionBlurCameraSelector;
}


Qt3DRender::QFrameGraphNode *QgsFrameGraph::constructRubberBandsPass()
{
  mRubberBandsCameraSelector = new Qt3DRender::QCameraSelector;
  mRubberBandsCameraSelector->setObjectName( "RubberBands Pass CameraSelector" );
  mRubberBandsCameraSelector->setCamera( mMainCamera );

  mRubberBandsLayerFilter = new Qt3DRender::QLayerFilter( mRubberBandsCameraSelector );
  mRubberBandsLayerFilter->addLayer( mRubberBandsLayer );

  mRubberBandsStateSet = new Qt3DRender::QRenderStateSet( mRubberBandsLayerFilter );
  Qt3DRender::QDepthTest *depthTest = new Qt3DRender::QDepthTest;
  depthTest->setDepthFunction( Qt3DRender::QDepthTest::Always );
  mRubberBandsStateSet->addRenderState( depthTest );

  // Here we attach our drawings to the render target also used by forward pass.
  // This is kind of okay, but as a result, post-processing effects get applied
  // to rubber bands too. Ideally we would want them on top of everything.
  mRubberBandsRenderTargetSelector = new Qt3DRender::QRenderTargetSelector( mRubberBandsStateSet );
  QgsForwardRenderView *forwardRenderView = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  mRubberBandsRenderTargetSelector->setTarget( forwardRenderView->renderTargetSelector()->target() );

  return mRubberBandsCameraSelector;
}



void QgsFrameGraph::constructDepthRenderPass()
{
  // depth buffer render to copy pass
  // Create a render target output for rendering color.
  Qt3DRender::QRenderTargetOutput *colorOutput = new Qt3DRender::QRenderTargetOutput;
  colorOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );

  // Create a texture to render into.
  Qt3DRender::QTexture2D *colorTexture = new Qt3DRender::QTexture2D( colorOutput );
  colorTexture->setSize( mSize.width(), mSize.height() );
  colorTexture->setFormat( Qt3DRender::QAbstractTexture::RGB8_UNorm );
  colorTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  colorTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );
  colorTexture->setObjectName( DEPTH_RENDERVIEW + "::ColorTarget" );

  // Hook the texture up to our output, and the output up to this object.
  colorOutput->setTexture( colorTexture );

  Qt3DRender::QRenderTargetOutput *depthOutput = new Qt3DRender::QRenderTargetOutput;

  depthOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Depth );
  Qt3DRender::QTexture2D *depthTexture = new Qt3DRender::QTexture2D( depthOutput );
  depthTexture->setSize( mSize.width(), mSize.height() );
  depthTexture->setFormat( Qt3DRender::QAbstractTexture::DepthFormat );
  depthTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  depthTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );
  depthTexture->setComparisonFunction( Qt3DRender::QAbstractTexture::CompareLessEqual );
  depthTexture->setComparisonMode( Qt3DRender::QAbstractTexture::CompareRefToTexture );
  depthTexture->setObjectName( DEPTH_RENDERVIEW + "::DepthTarget" );

  depthOutput->setTexture( depthTexture );

  QgsDepthRenderView *depthRenderView = new QgsDepthRenderView( this, mMainCamera );
  depthRenderView->setTargetOutputs( { colorOutput, depthOutput } );
  registerRenderView( depthRenderView, DEPTH_RENDERVIEW );

  // entity used to draw the depth texture and convert it to rgb image
  QgsAbstractRenderView *forwardRenderView = renderView( QgsFrameGraph::FORWARD_RENDERVIEW );
  Qt3DRender::QTexture2D *forwardDepthTexture = forwardRenderView->outputTexture( Qt3DRender::QRenderTargetOutput::Depth );
  Qt3DCore::QEntity *quad = new QgsDepthEntity( forwardDepthTexture, depthRenderView->layerToFilter(), mRootEntity );
  quad->setParent( mRootEntity );
}

Qt3DRender::QRenderCapture *QgsFrameGraph::depthRenderCapture()
{
  QgsDepthRenderView *depthRenderView = dynamic_cast<QgsDepthRenderView *>( renderView( DEPTH_RENDERVIEW ) );
  if ( depthRenderView )
    return depthRenderView->renderCapture();

  return nullptr;
}

QgsFrameGraph::QgsFrameGraph( QSurface *surface, QSize s, Qt3DRender::QCamera *mainCamera, Qt3DCore::QEntity *root )
  : Qt3DCore::QEntity( root )
  , mSize( s )
{

  // general overview of how the frame graph looks:
  //
  //  +------------------------+    using window or
  //  | QRenderSurfaceSelector |   offscreen surface
  //  +------------------------+
  //             |
  //  +-----------+
  //  | QViewport | (0,0,1,1)
  //  +-----------+
  //             |
  //     +--------------------------+-------------------+-----------------+
  //     |                          |                   |                 |
  // +--------------------+ +--------------+ +-----------------+ +-----------------+
  // | two forward passes | | shadows pass | |  depth buffer   | | post-processing |
  // |  (solid objects    | |              | | processing pass | |    passes       |
  // |  and transparent)  | +--------------+ +-----------------+ +-----------------+
  // +--------------------+
  //
  // Notes:
  // - depth buffer processing pass is used whenever we need depth map information
  //   (for camera navigation) and it converts depth texture to a color texture
  //   so that we can capture it with QRenderCapture - currently it is unable
  //   to capture depth buffer, only colors (see QTBUG-65155)
  // - there are multiple post-processing passes that take rendered output
  //   of the scene, optionally apply effects (add shadows, ambient occlusion,
  //   eye dome lighting) and finally output to the given surface
  // - there may be also two more passes when 3D axis is shown - see Qgs3DAxis

  mRootEntity = root;
  mMainCamera = mainCamera;

  mPreviewLayer = new Qt3DRender::QLayer;
  mRubberBandsLayer = new Qt3DRender::QLayer;

  mRubberBandsLayer->setObjectName( "mRubberBandsLayer" );

  mPreviewLayer->setRecursive( true );
  mRubberBandsLayer->setRecursive( true );

  mRenderSurfaceSelector = new Qt3DRender::QRenderSurfaceSelector;

  QObject *surfaceObj = dynamic_cast< QObject *  >( surface );
  Q_ASSERT( surfaceObj );

  mRenderSurfaceSelector->setSurface( surfaceObj );
  mRenderSurfaceSelector->setExternalRenderTargetSize( mSize );

  mMainViewPort = new Qt3DRender::QViewport( mRenderSurfaceSelector );
  mMainViewPort->setNormalizedRect( QRectF( 0.0f, 0.0f, 1.0f, 1.0f ) );

  // Forward render
  constructForwardRenderPass();

  // rubber bands (they should be always on top)
  Qt3DRender::QFrameGraphNode *rubberBandsPass = constructRubberBandsPass();
  rubberBandsPass->setObjectName( "rubberBandsPass" );
  rubberBandsPass->setParent( mMainViewPort );

  // shadow rendering pass
  constructShadowRenderPass();

  // depth buffer processing
  constructDepthRenderPass();

  // Ambient occlusion factor render pass
  Qt3DRender::QFrameGraphNode *ambientOcclusionFactorRender = constructAmbientOcclusionRenderPass();
  ambientOcclusionFactorRender->setParent( mMainViewPort );

  Qt3DRender::QFrameGraphNode *ambientOcclusionBlurPass = constructAmbientOcclusionBlurPass();
  ambientOcclusionBlurPass->setParent( mMainViewPort );

  // post process
  Qt3DRender::QFrameGraphNode *postprocessingPass = constructPostprocessingPass();
  postprocessingPass->setParent( mMainViewPort );
  postprocessingPass->setObjectName( "PostProcessingPass" );

  mRubberBandsRootEntity = new Qt3DCore::QEntity( mRootEntity );
  mRubberBandsRootEntity->setObjectName( "mRubberBandsRootEntity" );
  mRubberBandsRootEntity->addComponent( mRubberBandsLayer );

  Qt3DRender::QParameter *depthMapIsDepthParam = new Qt3DRender::QParameter( "isDepth", true );
  Qt3DRender::QParameter *shadowMapIsDepthParam = new Qt3DRender::QParameter( "isDepth", true );

  QgsAbstractRenderView *forwardRenderView = renderView( QgsFrameGraph::FORWARD_RENDERVIEW );
  Qt3DRender::QTexture2D *forwardDepthTexture = forwardRenderView->outputTexture( Qt3DRender::QRenderTargetOutput::Depth );
  mDebugDepthMapPreviewQuad = this->addTexturePreviewOverlay( forwardDepthTexture, QPointF( 0.9f, 0.9f ), QSizeF( 0.1, 0.1 ), QVector<Qt3DRender::QParameter *> { depthMapIsDepthParam } );

  QgsAbstractRenderView *shadowRenderView = renderView( QgsFrameGraph::SHADOW_RENDERVIEW );
  Qt3DRender::QTexture2D *shadowDepthTexture = shadowRenderView->outputTexture( Qt3DRender::QRenderTargetOutput::Depth );
  mDebugShadowMapPreviewQuad = this->addTexturePreviewOverlay( shadowDepthTexture, QPointF( 0.9f, 0.9f ), QSizeF( 0.1, 0.1 ), QVector<Qt3DRender::QParameter *> { shadowMapIsDepthParam } );

  mDebugDepthMapPreviewQuad->setEnabled( false );
  mDebugShadowMapPreviewQuad->setEnabled( false );

}

void QgsFrameGraph::unregisterRenderView( const QString &name )
{
  QgsAbstractRenderView *renderView = mRenderViewMap [name];
  if ( renderView )
  {
    renderView->topGraphNode()->setParent( ( QNode * )nullptr );
    mRenderViewMap.remove( name );
  }
}

bool QgsFrameGraph::registerRenderView( QgsAbstractRenderView *renderView, const QString &name, Qt3DRender::QFrameGraphNode *topNode )
{
  bool out;
  if ( mRenderViewMap [name] == nullptr )
  {
    mRenderViewMap [name] = renderView;
    if ( topNode )
      renderView->topGraphNode()->setParent( topNode );
    else
      renderView->topGraphNode()->setParent( mMainViewPort );
    out = true;
  }
  else
    out = false;

  return out;
}

void QgsFrameGraph::setEnableRenderView( const QString &name, bool enable )
{
  if ( mRenderViewMap [name] )
  {
    mRenderViewMap [name]->enableSubTree( enable );
  }
}

QgsAbstractRenderView *QgsFrameGraph::renderView( const QString &name )
{
  return mRenderViewMap [name];
}

bool QgsFrameGraph::isRenderViewEnabled( const QString &name )
{
  return mRenderViewMap [name] != nullptr && mRenderViewMap [name]->isSubTreeEnabled();
}

QgsPreviewQuad *QgsFrameGraph::addTexturePreviewOverlay( Qt3DRender::QTexture2D *texture, const QPointF &centerTexCoords, const QSizeF &sizeTexCoords, QVector<Qt3DRender::QParameter *> additionalShaderParameters )
{
  QgsPreviewQuad *previewQuad = new QgsPreviewQuad( texture, centerTexCoords, sizeTexCoords, additionalShaderParameters );
  previewQuad->addComponent( mPreviewLayer );
  previewQuad->setParent( mRootEntity );
  mPreviewQuads.push_back( previewQuad );
  return previewQuad;
}


QString QgsFrameGraph::dumpFrameGraph() const
{
  QObject *top = mRenderSurfaceSelector;
  while ( top->parent() && dynamic_cast<Qt3DRender::QFrameGraphNode *>( top->parent() ) )
    top = top->parent();

  QgsFgUtils::FgDumpContext context;
  context.lowestId = mMainCamera->id().id();
  QStringList strList = QgsFgUtils::dumpFrameGraph( dynamic_cast<Qt3DRender::QFrameGraphNode *>( top ), context );

  return strList.join( "\n" ) + QString( "\n" );
}

QString QgsFrameGraph::dumpSceneGraph() const
{
  QStringList strList = QgsFgUtils::dumpSceneGraph( mRootEntity, QgsFgUtils::FgDumpContext() );
  return strList.join( "\n" ) + QString( "\n" );
}

void QgsFrameGraph::setClearColor( const QColor &clearColor )
{
  QgsForwardRenderView *forwardRenderView = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  if ( forwardRenderView )
    forwardRenderView->setClearColor( clearColor );
}

void QgsFrameGraph::setAmbientOcclusionEnabled( bool enabled )
{
  mAmbientOcclusionEnabled = enabled;
  mAmbientOcclusionRenderEntity->setEnabled( enabled );
  mPostprocessingEntity->setAmbientOcclusionEnabled( enabled );
}

void QgsFrameGraph::setAmbientOcclusionIntensity( float intensity )
{
  mAmbientOcclusionIntensity = intensity;
  mAmbientOcclusionRenderEntity->setIntensity( intensity );
}

void QgsFrameGraph::setAmbientOcclusionRadius( float radius )
{
  mAmbientOcclusionRadius = radius;
  mAmbientOcclusionRenderEntity->setRadius( radius );
}

void QgsFrameGraph::setAmbientOcclusionThreshold( float threshold )
{
  mAmbientOcclusionThreshold = threshold;
  mAmbientOcclusionRenderEntity->setThreshold( threshold );
}

void QgsFrameGraph::setFrustumCullingEnabled( bool enabled )
{
  QgsForwardRenderView *forwardRenderView = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  if ( forwardRenderView )
    forwardRenderView->enableFrustumCulling( enabled );
}

void QgsFrameGraph::setupEyeDomeLighting( bool enabled, double strength, int distance )
{
  mEyeDomeLightingEnabled = enabled;
  mEyeDomeLightingStrength = strength;
  mEyeDomeLightingDistance = distance;
  mPostprocessingEntity->setEyeDomeLightingEnabled( enabled );
  mPostprocessingEntity->setEyeDomeLightingStrength( strength );
  mPostprocessingEntity->setEyeDomeLightingDistance( distance );
}

void QgsFrameGraph::setupShadowMapDebugging( bool enabled, Qt::Corner corner, double size )
{
  mDebugShadowMapPreviewQuad->setEnabled( enabled );
  if ( enabled )
  {
    switch ( corner )
    {
      case Qt::Corner::TopRightCorner:
        mDebugShadowMapPreviewQuad->setViewPort( QPointF( 1.0f - size / 2, 0.0f + size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
      case Qt::Corner::TopLeftCorner:
        mDebugShadowMapPreviewQuad->setViewPort( QPointF( 0.0f + size / 2, 0.0f + size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
      case Qt::Corner::BottomRightCorner:
        mDebugShadowMapPreviewQuad->setViewPort( QPointF( 1.0f - size / 2, 1.0f - size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
      case Qt::Corner::BottomLeftCorner:
        mDebugShadowMapPreviewQuad->setViewPort( QPointF( 0.0f + size / 2, 1.0f - size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
    }
  }
}

void QgsFrameGraph::setupDepthMapDebugging( bool enabled, Qt::Corner corner, double size )
{
  mDebugDepthMapPreviewQuad->setEnabled( enabled );

  if ( enabled )
  {
    switch ( corner )
    {
      case Qt::Corner::TopRightCorner:
        mDebugDepthMapPreviewQuad->setViewPort( QPointF( 1.0f - size / 2, 0.0f + size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
      case Qt::Corner::TopLeftCorner:
        mDebugDepthMapPreviewQuad->setViewPort( QPointF( 0.0f + size / 2, 0.0f + size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
      case Qt::Corner::BottomRightCorner:
        mDebugDepthMapPreviewQuad->setViewPort( QPointF( 1.0f - size / 2, 1.0f - size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
      case Qt::Corner::BottomLeftCorner:
        mDebugDepthMapPreviewQuad->setViewPort( QPointF( 0.0f + size / 2, 1.0f - size / 2 ), 0.5 * QSizeF( size, size ) );
        break;
    }
  }
}

void QgsFrameGraph::setSize( QSize s )
{
  mSize = s;
  QgsForwardRenderView *forwardRenderView = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  if ( forwardRenderView )
    forwardRenderView->updateTargetOutputSize( mSize.width(), mSize.height() );

  QgsDepthRenderView *depthRenderView = dynamic_cast<QgsDepthRenderView *>( renderView( DEPTH_RENDERVIEW ) );
  if ( depthRenderView )
    depthRenderView->updateTargetOutputSize( mSize.width(), mSize.height() );

  mRenderCaptureColorTexture->setSize( mSize.width(), mSize.height() );
  mRenderCaptureDepthTexture->setSize( mSize.width(), mSize.height() );
  mRenderSurfaceSelector->setExternalRenderTargetSize( mSize );

  mAmbientOcclusionRenderTexture->setSize( mSize.width(), mSize.height() );
  mAmbientOcclusionBlurTexture->setSize( mSize.width(), mSize.height() );
}

void QgsFrameGraph::setRenderCaptureEnabled( bool enabled )
{
  if ( enabled == mRenderCaptureEnabled )
    return;
  mRenderCaptureEnabled = enabled;
  mRenderCaptureTargetSelector->setEnabled( mRenderCaptureEnabled );
}

void QgsFrameGraph::setDebugOverlayEnabled( bool enabled )
{
  QgsForwardRenderView *forwardRenderView = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  if ( forwardRenderView )
    forwardRenderView->enableDebugOverlay( enabled );
}
