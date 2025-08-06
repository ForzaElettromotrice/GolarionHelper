package org.golarion;

import javafx.application.Application;

/**
 * Launcher class to properly start JavaFX application from a fat JAR.
 * This avoids the "JavaFX runtime components are missing" error.
 */
public class Launcher {
    public static void main(String[] args) {
        Application.launch(App.class, args);
    }
}