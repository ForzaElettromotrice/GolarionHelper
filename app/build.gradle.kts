import org.gradle.internal.os.OperatingSystem


plugins {
    java
    application
    alias(libs.plugins.javafx)
    alias(libs.plugins.jlink)
}

repositories {
    mavenCentral()
}

dependencies {
    implementation("org.openjfx:javafx-controls:${libs.versions.javafx.get()}")
    implementation("org.openjfx:javafx-graphics:${libs.versions.javafx.get()}")
    implementation("org.openjfx:javafx-base:${libs.versions.javafx.get()}")
    implementation("com.google.code.gson:gson:2.11.0")

    compileOnly(libs.lombok)
    annotationProcessor(libs.lombok)
}

application {
    mainClass = "org.golarion.App"
    mainModule = "org.golarion"
}

javafx {
    version = libs.versions.javafx.get()
    modules = listOf("javafx.controls", "javafx.graphics", "javafx.base")
}

tasks {
    java {
        toolchain {
            languageVersion = JavaLanguageVersion.of(21)
        }
    }
}

jlink {
    imageZip = project.file("${layout.buildDirectory}/distributions/app-${javafx.platform.classifier}.zip")
    options = listOf("--strip-debug", "--compress", "zip-6", "--no-header-files", "--no-man-pages")
    launcher {
        name = "golarion-app"
    }
    jpackage {
        imageName = "Golarion"
        installerName = "Golarion-Installer"
        appVersion = "1.0.0"

        val os = OperatingSystem.current()
        when {
            os.isWindows -> {
                installerType = "msi"
                installerOptions = listOf(
                    "--win-per-user-install",
                    "--win-dir-chooser",
                    "--win-menu",
                    "--win-shortcut"
                )
            }

            else -> throw GradleException("Unsupported OS: ${os.name}")
        }
    }
}