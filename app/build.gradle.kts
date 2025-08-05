plugins {
    java
    application
    alias(libs.plugins.shadow)
}

repositories {
    mavenCentral()
}

dependencies {
    val osName = System.getProperty("os.name").lowercase()
    val javafxPlatform = when {
        osName.contains("win") -> "win"
        osName.contains("mac") -> "mac"
        osName.contains("linux") -> "linux"
        else -> throw GradleException("Unknown OS: $osName")
    }

    implementation("org.openjfx:javafx-controls:${libs.versions.javafx.get()}:$javafxPlatform")
    implementation("org.openjfx:javafx-graphics:${libs.versions.javafx.get()}:$javafxPlatform")
    implementation("org.openjfx:javafx-base:${libs.versions.javafx.get()}:$javafxPlatform")

    compileOnly(libs.lombok)
    annotationProcessor(libs.lombok)
}

application {
    mainClass = "org.golarion.Launcher"
}

tasks {
    java {
        toolchain {
            languageVersion = JavaLanguageVersion.of(21)
        }
    }

    shadowJar {
        archiveClassifier.set("")
        manifest {
            attributes("Main-Class" to "org.golarion.Launcher")
        }
    }
}