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
#include "qgs3dutils.h"
#include "qgsfgutils.h"
#include "qgsabstractrenderview.h"

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
#include "qgsdebugtexturerenderview.h"
#include "qgsdebugtextureentity.h"
#include "qgsambientocclusionrenderview.h"
#include "qgspostprocessingrenderview.h"

const QString QgsFrameGraph::FORWARD_RENDERVIEW = "forward";
const QString QgsFrameGraph::SHADOW_RENDERVIEW = "shadow";
const QString QgsFrameGraph::AXIS3D_RENDERVIEW = "3daxis";
const QString QgsFrameGraph::DEPTH_RENDERVIEW = "depth";
const QString QgsFrameGraph::DEBUG_RENDERVIEW = "debug_texture";
const QString QgsFrameGraph::AO_RENDERVIEW = "ambient_occlusion";
const QString QgsFrameGraph::POSTPROC_RENDERVIEW = "post_processing";

QgsFrameGraph::~QgsFrameGraph()
{
  for ( QgsAbstractRenderView *rv : qAsConst( mRenderViewMap ) )
  {
    rv->deleteLater();
  }
}

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

void QgsFrameGraph::constructPostprocessingPass()
{
  QgsShadowRenderView *srv = dynamic_cast<QgsShadowRenderView *>( renderView( QgsFrameGraph::SHADOW_RENDERVIEW ) );
  QgsForwardRenderView *frv = dynamic_cast<QgsForwardRenderView *>( renderView( QgsFrameGraph::FORWARD_RENDERVIEW ) );
  QgsAmbientOcclusionRenderView *aorv = dynamic_cast<QgsAmbientOcclusionRenderView *>( renderView( QgsFrameGraph::AO_RENDERVIEW ) );

  // create post processing render view and register it
  QgsPostprocessingRenderView *pprv = new QgsPostprocessingRenderView( this
      , srv
      , frv
      , aorv
      , mSize
      , mRootEntity );
  registerRenderView( pprv, POSTPROC_RENDERVIEW );

  // create debug texture render view and register it (will be detach from main framegraph and attach to postprocessing renderview)
  QgsDebugTextureRenderView *dtrv = new QgsDebugTextureRenderView( this, frv->mainCamera() );
  registerRenderView( dtrv, DEBUG_RENDERVIEW );

  // add debug texture render view to post processing subpasses (before the render capture one)
  QVector<Qt3DRender::QFrameGraphNode *> subPasses = pprv->subPasses();
  subPasses.insert( subPasses.length() - 1, mRenderViewMap[DEBUG_RENDERVIEW]->topGraphNode() );
  pprv->setSubPasses( subPasses );
}

void QgsFrameGraph::constructAmbientOcclusionRenderPass()
{
  // Create a texture to render into.
  Qt3DRender::QRenderTargetOutput *colorTargetOutput = new Qt3DRender::QRenderTargetOutput;
  colorTargetOutput->setAttachmentPoint( Qt3DRender::QRenderTargetOutput::Color0 );

  Qt3DRender::QTexture2D *renderTexture = new Qt3DRender::QTexture2D( colorTargetOutput );
  renderTexture->setSize( mSize.width(), mSize.height() );
  renderTexture->setFormat( Qt3DRender::QAbstractTexture::R32F );
  renderTexture->setMinificationFilter( Qt3DRender::QAbstractTexture::Linear );
  renderTexture->setMagnificationFilter( Qt3DRender::QAbstractTexture::Linear );
  renderTexture->setObjectName( AO_RENDERVIEW + "::ColorTarget" );
  colorTargetOutput->setTexture( renderTexture );

  QgsAbstractRenderView *forwardRenderView = renderView( QgsFrameGraph::FORWARD_RENDERVIEW );
  Qt3DRender::QTexture2D *forwardDepthTexture = forwardRenderView->outputTexture( Qt3DRender::QRenderTargetOutput::Depth );

  QgsAmbientOcclusionRenderView *aorv = new QgsAmbientOcclusionRenderView( this, mMainCamera, mSize, forwardDepthTexture, mRootEntity );
  aorv->setTargetOutputs( { colorTargetOutput } );
  registerRenderView( aorv, AO_RENDERVIEW );
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

  QgsAbstractRenderView *forwardRenderView = renderView( QgsFrameGraph::FORWARD_RENDERVIEW );
  Qt3DRender::QTexture2D *forwardDepthTexture = forwardRenderView->outputTexture( Qt3DRender::QRenderTargetOutput::Depth );

  QgsDepthRenderView *depthRenderView = new QgsDepthRenderView( this, mMainCamera, forwardDepthTexture, mRootEntity );
  depthRenderView->setTargetOutputs( { colorOutput, depthOutput } );
  registerRenderView( depthRenderView, DEPTH_RENDERVIEW );
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

  mRubberBandsLayer = new Qt3DRender::QLayer;
  mRubberBandsLayer->setObjectName( "mRubberBandsLayer" );
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
  constructAmbientOcclusionRenderPass();

  // post process
  constructPostprocessingPass();

  mRubberBandsRootEntity = new Qt3DCore::QEntity( mRootEntity );
  mRubberBandsRootEntity->setObjectName( "mRubberBandsRootEntity" );
  mRubberBandsRootEntity->addComponent( mRubberBandsLayer );
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

Qt3DRender::QLayer *QgsFrameGraph::filterLayer( const QString &name )
{
  return mRenderViewMap [name]->layerToFilter();
}

bool QgsFrameGraph::isRenderViewEnabled( const QString &name )
{
  return mRenderViewMap [name] != nullptr && mRenderViewMap [name]->isSubTreeEnabled();
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

void QgsFrameGraph::setFrustumCullingEnabled( bool enabled )
{
  QgsForwardRenderView *forwardRenderView = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  if ( forwardRenderView )
    forwardRenderView->enableFrustumCulling( enabled );
}

void QgsFrameGraph::setupEyeDomeLighting( bool enabled, double strength, int distance )
{
  QgsPostprocessingRenderView *pprv = dynamic_cast<QgsPostprocessingRenderView *>( renderView( QgsFrameGraph::POSTPROC_RENDERVIEW ) );
  if ( pprv )
    pprv->setupEyeDomeLighting( enabled, strength, distance );
}

void QgsFrameGraph::setSize( QSize s )
{
  mSize = s;

  for ( QgsAbstractRenderView *renderView : qAsConst( mRenderViewMap ) )
  {
    renderView->updateTargetOutputSize( mSize.width(), mSize.height() );
  }

  mRenderSurfaceSelector->setExternalRenderTargetSize( mSize );
}

Qt3DRender::QRenderCapture *QgsFrameGraph::renderCapture()
{
  QgsPostprocessingRenderView *pprv = dynamic_cast<QgsPostprocessingRenderView *>( renderView( QgsFrameGraph::POSTPROC_RENDERVIEW ) );
  if ( pprv )
    return pprv->renderCapture();
  return nullptr;
}

void QgsFrameGraph::setOffScreenRenderCaptureEnabled( bool enabled )
{
  QgsPostprocessingRenderView *pprv = dynamic_cast<QgsPostprocessingRenderView *>( renderView( QgsFrameGraph::POSTPROC_RENDERVIEW ) );
  if ( pprv )
    pprv->setOffScreenRenderCaptureEnabled( enabled );
}

void QgsFrameGraph::setDebugOverlayEnabled( bool enabled )
{
  QgsForwardRenderView *forwardRenderView = dynamic_cast<QgsForwardRenderView *>( renderView( FORWARD_RENDERVIEW ) );
  if ( forwardRenderView )
    forwardRenderView->enableDebugOverlay( enabled );
}

Qt3DCore::QEntity *QgsFrameGraph::rubberBandsRootEntity()
{
  return mRubberBandsRootEntity;
}
