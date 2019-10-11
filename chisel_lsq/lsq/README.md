Chisel LSQ
=======================
## Required IntelliJ Plugins
1. Scala
2. sbt
## How to open the project in IntelliJ IDEA

1. Open IntelliJ IDEA and click **Import Project**.
2. Open dhls/chisel_lsq/lsq/build.sbt.
3. This should open **Import Project from sbt** dialog box.
4. Click OK.

## How to build jar

### Follow these steps before you generate the jar for the first time.

1. Go to **File** -> **Project Structure**.
2. Select **Artifacts** from Project Settings.
3. Add build artifact by clicking on the  **+** sign, **JAR**-> **From modules with dependancies...**.
4. Select **lsq** as the module and **Main** as the Main Class in **Create JAR from Modules** dialog box.
5. Click OK.
6. Click OK.

### To build the jar, 

1. Build -> Build Artifacts...
2. lsq:jar -> Build


lsq.jar can be found in dhls/chisel_lsq/lsq/out/artifacts/lsq_jar.
