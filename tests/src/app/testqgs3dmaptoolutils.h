/***************************************************************************
     testqgs3dmaptoolutils.h
     ---------------------
    begin                : December 2025
    copyright            : (C) 2025 by Oslandia
    email                : benoit dot de dot mezzo at oslandia dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgs3dmaptool.h"

#include <QMouseEvent>

/**
 * \ingroup UnitTests
 */
class TestQgs3DMapToolUtils
{
  public:
    TestQgs3DMapToolUtils( Qgs3DMapTool *mapTool, const QSize &screenSize )
      : mMapTool( mapTool )
    {
      mCenter.setX( screenSize.width() / 2 );
      mCenter.setY( screenSize.height() / 2 );
    }

    void mouseMove( QPoint middlePos, Qt::KeyboardModifiers stateKey = Qt::KeyboardModifiers() )
    {
      mouseMove( middlePos.x(), middlePos.y(), stateKey );
    }

    void mouseMove( double middlePosX, double middlePosY, Qt::KeyboardModifiers stateKey = Qt::KeyboardModifiers() )
    {
      QMouseEvent e( QEvent::MouseMove, QPoint( mCenter.x() + middlePosX, mCenter.y() + middlePosY ), Qt::NoButton, Qt::NoButton, stateKey );
      mMapTool->mouseMoveEvent( &e );
    }

    void mousePress( QPoint middlePos, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = Qt::KeyboardModifiers() )
    {
      mousePress( middlePos.x(), middlePos.y(), button, stateKey );
    }

    void mousePress( double middlePosX, double middlePosY, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = Qt::KeyboardModifiers() )
    {
      QMouseEvent e( QEvent::MouseButtonPress, QPoint( mCenter.x() + middlePosX, mCenter.y() + middlePosY ), button, button, stateKey );
      mMapTool->mousePressEvent( &e );
    }

    void mouseRelease( QPoint middlePos, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = Qt::KeyboardModifiers() )
    {
      mouseRelease( middlePos.x(), middlePos.y(), button, stateKey );
    }

    void mouseRelease( double middlePosX, double middlePosY, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = Qt::KeyboardModifiers() )
    {
      QMouseEvent e( QEvent::MouseButtonRelease, QPoint( mCenter.x() + middlePosX, mCenter.y() + middlePosY ), button, Qt::MouseButton(), stateKey );
      mMapTool->mouseReleaseEvent( &e );
    }

    void mouseClick( QPoint middlePos, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = Qt::KeyboardModifiers() )
    {
      mouseClick( middlePos.x(), middlePos.y(), button, stateKey );
    }

    void mouseClick( double middlePosX, double middlePosY, Qt::MouseButton button, Qt::KeyboardModifiers stateKey = Qt::KeyboardModifiers() )
    {
      mousePress( middlePosX, middlePosY, button, stateKey );
      mouseRelease( middlePosX, middlePosY, button, stateKey );
    }

    void keyClick( int key, Qt::KeyboardModifiers stateKey = Qt::KeyboardModifiers() )
    {
      QKeyEvent e1( QEvent::KeyPress, key, stateKey );
      mMapTool->keyPressEvent( &e1 );

      QKeyEvent e2( QEvent::KeyRelease, key, stateKey );
      mMapTool->keyReleaseEvent( &e2 );
    }

  private:
    Qgs3DMapTool *mMapTool = nullptr;
    QPoint mCenter;
};
