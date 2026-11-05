// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
package org.qtproject.qt.android.multimedia.qffmpeg;

import android.annotation.TargetApi;
import android.os.Build;
import android.util.Log;

import java.lang.ref.Cleaner;

import org.qtproject.qt.android.UsedFromNativeCode;

// Wrapper around the native counterpart QAndroidCameraActivation.
//
// In the case this class is never signalled with a success/failure result,
// we make sure garbage collector will eventually destroy the underlying
// C++ object.
class QtCameraActivation {
    private static native void onActivationFinished(
        long activationPtr,
        boolean success,
        String errorMessage);

    // Separate from QtCameraActivation, so that the cleanup can run once it is unreachable.
    private static final class State implements Runnable {
        private long mActivationPtr = 0;

        State(long activationPtr) {
            mActivationPtr = activationPtr;
        }

        synchronized boolean report(boolean success, String errorMessage) {
            if (mActivationPtr == 0)
                return false;

            final long activationPtr = mActivationPtr;
            mActivationPtr = 0;
            onActivationFinished(activationPtr, success, errorMessage);
            return true;
        }

        @Override
        public void run() {
            if (report(false, "Failed to start camera"))
                Log.w(QtCamera2.LOG_TAG, "Camera activation was abandoned without a result");
        }
    }

    // Separate class, so that Cleaner is never loaded before Android 13.
    @TargetApi(Build.VERSION_CODES.TIRAMISU)
    private static final class ModernCleanup {
        private static final Cleaner sCleaner = Cleaner.create();

        static Object register(Object owner, State state) {
            return sCleaner.register(owner, state);
        }
    }

    private static final class LegacyCleanup {
        private final State mState;

        LegacyCleanup(State state) {
            mState = state;
        }

        @Override
        @SuppressWarnings("deprecation")
        protected void finalize() throws Throwable {
            try {
                mState.run();
            } finally {
                super.finalize();
            }
        }
    }

    private final State mState;
    // Keeps the cleanup alive for as long as this object is reachable.
    private final Object mCleanup;

    @UsedFromNativeCode
    QtCameraActivation(long activationPtr) {
        assert(activationPtr != 0);
        mState = new State(activationPtr);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
            mCleanup = ModernCleanup.register(this, mState);
        else
            mCleanup = new LegacyCleanup(mState);
    }

    boolean reportSuccess() {
        return mState.report(true, "");
    }

    boolean reportFailure(String errorMessage) {
        return mState.report(false, errorMessage);
    }
}
