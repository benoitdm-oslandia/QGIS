/***************************************************************************
  qgs3dmaptoolstreetview.h
  --------------------------------------
  Date                 : May 2026
  Copyright            : (C) 2026 by Benoit De Mezzo
  Email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGS3DMAPTOOLSTREETVIEW_H
#define QGS3DMAPTOOLSTREETVIEW_H

#include <memory>

#include "qgs3dmaptool.h"
#include "qgscamerapose.h"
#include "qgspoint.h"

class QgsRubberBand3D;
class QgsCameraPose;
class QTimer;

class Qgs3DMapToolStreetView : public Qgs3DMapTool
{
    Q_OBJECT

  public:
    Qgs3DMapToolStreetView( Qgs3DMapCanvas *canvas );
    ~Qgs3DMapToolStreetView() override;

    void activate() override;
    void deactivate() override;

    QCursor cursor() const override;

    //! Reset and start new
    void reset();

    //! Setup pin point.
    void setupMarker( const QgsPoint &point );

    void setupNavigation();

    //! Update values from settings
    void updateSettings();

    void navigateRightSide( float steps );
    void navigateForward( float steps );

    /**
     * Update camero position at the \a newCamPosInMap.
     *
     * Elevation will be computed according to terrain elevation at X/Y, vertical scale and terrain offset.
     */
    void updateNavigationCamera( const QgsPoint &newCamPosInMap );

  signals:
    void finished();

  private slots:
    void handleClick( const QPoint &screenPos );
    // void mousePressEvent( QMouseEvent *event ) override;
    void mouseReleaseEvent( QMouseEvent *event ) override;
    void mouseMoveEvent( QMouseEvent *event ) override;
    void keyPressEvent( QKeyEvent *event ) override;
    void mouseWheelEvent( QWheelEvent *event ) override;
    void refreshCamera();

  private:
    std::unique_ptr<QgsRubberBand3D> mRubberBand;

    bool mIsNavigating;
    bool mIsEnabled;
    bool mIgnoreNextMouseMove;
    QgsCameraPose mPreviousCameraPose;
    QgsPoint mMarkerPos;
    //QPoint mMouseMovePos;
    QTime mLastMarkerTime;
    QgsPoint mLastCamPosInMap;

    QTimer *mJumpTimer;
    QTime mJumpTime;
    double mJumpDefaultHeight = 5.0;
    double mJumpDefaultTime = 100; // msec
    double mJumpAgainTime = 0;
    double mJumpAgainHeight = 0;

    double jumpHeight( double t );
};

#endif // QGS3DMAPTOOLSTREETVIEW_H
