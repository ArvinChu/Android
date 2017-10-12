[TOC]
# Ant自动编译打包Android过程分析

标签（空格分隔）： ant android

---

## 1. 背景
Eclipse用起来虽然方便，但是编译打包android项目还是比较慢。当我们需要将应用打包发布到各个渠道，或者将APK编译过程集成到编译系统时，就不能使用Eclipse了，这个时候可以用Ant帮我们自动编译打包。
## 2. Ant安装
打开[Apache Ant官网][1]下载最新zip或tar.gz压缩包。下载之后将其解压到某个目录下，然后配置环境变量，将{ANT解压目录}/bin添加到path中。
打开命令行工具，输入ant -version验证是否安装成功。
## 3. Android SDK安装
参考http://www.cnblogs.com/zoupeiyang/p/4034517.html，下载安装JDK、Android SDK，不用安装Eclipse及ADT。配置环境变量，将{Android SDK安装目录}/platform-tools、{Android SDK安装目录}/tools添加到path中。
## 4. 对Android项目提供Ant支持
打开命令行工具，切换路径到Android项目所在的目录，执行命令：
> **android update project --name ProjectName --target TargetBuildLevel --path ProjectPath --subprojects**<br>
--name(-n): 指定项目名称(可选)。如果提供，这个name将作为编译工程生成的.apk文件的文件名<br>
--target(-t): 指定构建目标对应的SDK ID。例如选择Android 4.4，则target为19<br>
--path(-p): 指定工程目录的位置(./代表当前目录)，如果工程目录不存在，就新创建一个<br>
--subprojects: 表示该项目引用了子工程，需要将子工程的信息更新到当前项目。如果没有引用子工程，则不需要此项。

如果项目是library类型，则切换到子工程目录，执行以下命令：
> android update lib-project -p ./

android update命令会生产三个文件：`project.properties、local.properties、build.xml`<br>
project.properties主要配置：代码混淆文件的路径、构建Target、引用的Library工程<br>
local.properties主要配置：Android SDK路径<br>
build.xml：Ant默认构建文件
## 5. build.xml文件分析

 - 分析使用 android update 命令生成的 build.xml
```
<?xml version="1.0" encoding="UTF-8"?>
<project name="RccStu" default="help">

    <!-- The local.properties file is created and updated by the 'android' tool.
         It contains the path to the SDK. It should *NOT* be checked into
         Version Control Systems. -->
    <property file="local.properties" />

    <!-- The ant.properties file can be created by you. It is only edited by the
         'android' tool to add properties to it.
         This is the place to change some Ant specific build properties.
         Here are some properties you may want to change/update:

         source.dir
             The name of the source directory. Default is 'src'.
         out.dir
             The name of the output directory. Default is 'bin'.

         For other overridable properties, look at the beginning of the rules
         files in the SDK, at tools/ant/build.xml

         Properties related to the SDK location or the project target should
         be updated using the 'android' tool with the 'update' action.

         This file is an integral part of the build system for your
         application and should be checked into Version Control Systems.

         -->
    <property file="ant.properties" />

    <!-- if sdk.dir was not set from one of the property file, then
         get it from the ANDROID_HOME env var.
         This must be done before we load project.properties since
         the proguard config can use sdk.dir -->
    <property environment="env" />
    <condition property="sdk.dir" value="${env.ANDROID_HOME}">
        <isset property="env.ANDROID_HOME" />
    </condition>

    <!-- The project.properties file is created and updated by the 'android'
         tool, as well as ADT.

         This contains project specific properties such as project target, and library
         dependencies. Lower level build properties are stored in ant.properties
         (or in .classpath for Eclipse projects).

         This file is an integral part of the build system for your
         application and should be checked into Version Control Systems. -->
    <loadproperties srcFile="project.properties" />

    <!-- quick check on sdk.dir -->
    <fail
            message="sdk.dir is missing. Make sure to generate local.properties using 'android update project' or to inject it through the ANDROID_HOME environment variable."
            unless="sdk.dir"
    />

    <!--
        Import per project custom build rules if present at the root of the project.
        This is the place to put custom intermediary targets such as:
            -pre-build
            -pre-compile
            -post-compile (This is typically used for code obfuscation.
                           Compiled code location: ${out.classes.absolute.dir}
                           If this is not done in place, override ${out.dex.input.absolute.dir})
            -post-package
            -post-build
            -pre-clean
    -->
    <import file="custom_rules.xml" optional="true" />

    <!-- Import the actual build file.

         To customize existing targets, there are two options:
         - Customize only one target:
             - copy/paste the target into this file, *before* the
               <import> task.
             - customize it to your needs.
         - Customize the whole content of build.xml
             - copy/paste the content of the rules files (minus the top node)
               into this file, replacing the <import> task.
             - customize to your needs.

         ***********************
         ****** IMPORTANT ******
         ***********************
         In all cases you must update the value of version-tag below to read 'custom' instead of an integer,
         in order to avoid having your file be overridden by tools such as "android update project"
    -->
    <!-- version-tag: 1 -->
    <import file="${sdk.dir}/tools/ant/build.xml" />

</project>
```
这里我们重点关注两点：<br>
`<import file="custom_rules.xml" optional="true" />` 导入定制构建文件(可选)，需要我们在当前目录下自定义。可定义target列表如下，后面的内容会介绍如何使用：

> -pre-build<br>
> -pre-compile<br>
> -post-compile<br>
> -post-package<br>
> -post-build<br>
> -pre-clean<br>

`<import file="${sdk.dir}/tools/ant/build.xml" />` 导入Android工程构建文件，存放在Android SDK中
## 6. ant release流程分析
- 首先看看打包APK的大致步骤，做初步了解：

1. 用aapt命令生成R.java文件
2. 用aidl命令生成相应java文件
3. 用javac命令编译java源文件生成class文件
4. 用dx.bat将class文件转换成classes.dex文件
5. 用aapt命令生成资源包文件resources.ap_
6. 用apkbuilder.bat打包资源和classes.dex文件,生成unsigned.apk
7. 用jarsinger命令对apk认证,生成signed.apk

- 然后分析文件 ${sdk.dir}/tools/ant/build.xml，查看release的执行过程，按照target的执行顺序介绍：
> 这里有份Android ant-task的简要说明：http://tools.android.com/tech-docs/ant-tasks

-set-mode-check： 检测运行模式。如果同时运行debug/release/instrument中的两种或三种，则抛出异常

-set-release-mode：主要设置release模式下的一些属性

-release-obfuscation-check：检测是否开启代码混淆并设置相关属性

-pre-build：**构建前的处理，可以在custom_rules.xml文件中定义**

-check-env：检查Android SDK及ANT环境

-setup：设置项目类型（app、library、test、test-app）

-build-setup：

 - 删除编译临时目录
 - 设置工程相关属性
 - 创建编译过程中需要的目录
 - 解析工程Dependencies
 - **编译依赖的Library工程**

-code-gen：

 - 合并Library工程中的AndroidManifest到主工程中
 - 用aidl命令生成相应java文件、用aapt命令生成R.java文件

-pre-compile：**编译前的处理，可以在custom_rules.xml文件中定义**

-compile：

 - 用javac命令编译java源文件生成class文件
 - 如果是library工程则用jar命令生成jar包(名称为classes.jar)

-post-compile：**编译后的处理，可以在custom_rules.xml文件中定义**

-obfuscate：执行代码混淆

-dex：用dx.bat将class.jar文件转换成classes.dex文件

-crunch：用aapt命令处理png文件

-package-resources：用aapt命令生成资源包文件resources.ap_

-package：用apkbuilder.bat打包资源和classes.dex文件，生成unsigned.apk。***.so库文件也是在此打包的。**

-post-package：**打包后的处理，可以在custom_rules.xml文件中定义**

-release-prompt-for-password：输入签名文件的信息

-release-sign：签名

-post-build：**构建后的处理，可以在custom_rules.xml文件中定义**


----------
## 附录
```
<?xml version="1.0" encoding="UTF-8"?>

<!--
  Android ant-task说明：http://tools.android.com/tech-docs/ant-tasks
-->

<!-- This runs -package-release and -release-nosign first and then runs
	 only if release-sign is true (set in -release-check,
	 called by -release-no-sign)-->
<target name="release"
	depends="-set-release-mode, -release-obfuscation-check, -package, -post-package, -release-prompt-for-password, -release-nosign, -release-sign, -post-build"
	description="Builds the application in release mode.">

  <!-- 检测如果同时运行debug/release/instrument中的两种或三种，则抛出异常 -->
  <target name="-set-mode-check">
  	<fail if="build.is.mode.set"
  			message="Cannot run two different modes at the same time. If you are running more than one debug/release/instrument type targets, call them from different Ant calls." />
  </target>
  <target name="-set-release-mode" depends="-set-mode-check">
  	<property name="out.packaged.file" location="${out.absolute.dir}/${ant.project.name}-release-unsigned.apk" />
  	<property name="out.final.file" location="${out.absolute.dir}/${ant.project.name}-release.apk" />
  	<property name="build.is.mode.set" value="true" />

  	<!-- record the current build target -->
  	<property name="build.target" value="release" />

  	<property name="build.is.instrumented" value="false" />

  	<!-- release mode is only valid if the manifest does not explicitly
  		 set debuggable to true. default is false. -->
  	<xpath input="${manifest.abs.file}" expression="/manifest/application/@android:debuggable"
  			output="build.is.packaging.debug" default="false"/>

  	<!-- signing mode: release -->
  	<property name="build.is.signing.debug" value="false" />

  	<!-- Renderscript optimization level: aggressive -->
  	<property name="renderscript.opt.level" value="${renderscript.release.opt.level}" />

  	<if condition="${build.is.packaging.debug}">
  		<then>
  			<echo>*************************************************</echo>
  			<echo>****  Android Manifest has debuggable=true   ****</echo>
  			<echo>**** Doing DEBUG packaging with RELEASE keys ****</echo>
  			<echo>*************************************************</echo>
  		</then>
  		<else>
  			<!-- property only set in release mode.
  				 Useful for if/unless attributes in target node
  				 when using Ant before 1.8 -->
  			<property name="build.is.mode.release" value="true"/>
  		</else>
  	</if>
  </target>

  <!-- 检测是否开启代码混淆 -->
  <target name="-release-obfuscation-check">
  	<echo level="info">proguard.config is ${proguard.config}</echo>
    <!-- 如果设置属性build.is.mode.release和proguard.config，则proguard.enabled=true否则为false -->
  	<condition property="proguard.enabled" value="true" else="false">
  		<and>
  			<isset property="build.is.mode.release" />
  			<isset property="proguard.config" />
  		</and>
  	</condition>
  	<if condition="${proguard.enabled}">
  		<then>
  			<echo level="info">Proguard.config is enabled</echo>
  			<!-- Secondary dx input (jar files) is empty since all the
  				 jar files will be in the obfuscated jar -->
  			<path id="out.dex.jar.input.ref" />
  		</then>
  	</if>
  </target>


  <!-- 编译前的处理，需要在custom_rules.xml中定义 -->
  <!-- empty default pre-build target. Create a similar target in
  	 your build.xml and it'll be called instead of this one. -->
  <target name="-pre-build"/>

  <!-- 检查Android SDK及ANT环境 -->
  <!-- Basic Ant + SDK check -->
  <target name="-check-env">
      <checkenv />
  </target>

  <!-- 初始化 -->
  <!-- generic setup -->
  <target name="-setup" depends="-check-env">
    <echo level="info">Project Name: ${ant.project.name}</echo>
    <!-- 将Android工程的类型(app/library/test/test-app)赋值给project.type -->
    <gettype projectTypeOut="project.type" />

    <!-- sets a few boolean based on project.type
         to make the if task easier -->
    <condition property="project.is.library" value="true" else="false">
        <equals arg1="${project.type}" arg2="library" />
    </condition>
    <condition property="project.is.test" value="true" else="false">
        <equals arg1="${project.type}" arg2="test" />
    </condition>
    <condition property="project.is.testapp" value="true" else="false">
        <equals arg1="${project.type}" arg2="test-app" />
    </condition>

    <!-- 如果是test工程，设置tested.project.absolute.dir -->
    <!-- If a test project, resolve absolute path to tested project. -->
    <if condition="${project.is.test}">
        <then>
            <property name="tested.project.absolute.dir" location="${tested.project.dir}" />
        </then>
    </if>

    <!-- 将工程package赋值给project.app.package -->
    <!-- get the project manifest package -->
    <xpath input="${manifest.abs.file}"
            expression="/manifest/@package" output="project.app.package" />

  </target>

  <!-- 预编译处理 -->
  <!-- Pre build setup -->
  <target name="-build-setup" depends="-setup">
    <!-- find location of build tools -->
    <getbuildtools name="android.build.tools.dir" verbose="${verbose}" />

    <!-- read the previous build mode -->
    <property file="${out.build.prop.file}" />
    <!-- if empty the props won't be set, meaning it's a new build.
         To force a build, set the prop to empty values. -->
    <property name="build.last.target" value="" />
    <property name="build.last.is.instrumented" value="" />
    <property name="build.last.is.packaging.debug" value="" />
    <property name="build.last.is.signing.debug" value="" />

    <!-- If the "debug" build type changed, clear out the compiled code.
         This is to make sure the new BuildConfig.DEBUG value is picked up
         as javac can't deal with this type of change in its dependency computation. -->
    <if>
        <!-- 如果build.is.packaging.debug改变，删除out.classes.absolute.dir目录 -->
        <condition>
            <and>
                <length string="${build.last.is.packaging.debug}" trim="true" when="greater" length="0" />
                <not><equals
                        arg1="${build.is.packaging.debug}"
                        arg2="${build.last.is.packaging.debug}" /></not>
            </and>
        </condition>
        <then>
            <echo level="info">Switching between debug and non debug build: Deleting previous compilation output...</echo>
            <delete dir="${out.classes.absolute.dir}" verbose="${verbose}" />
        </then>
        <else>
            <!-- Else, we may still need to clean the code, for another reason.
                 special case for instrumented: if the previous build was
                 instrumented but not this one, clear out the compiled code -->
            <if>
                <!-- 如果上次编译instrument而这次不编译instrument，删除out.classes.absolute.dir目录 -->
                <condition>
                    <and>
                        <istrue value="${build.last.is.instrumented}" />
                        <isfalse value="${build.is.instrumented}" />
                    </and>
                </condition>
                <then>
                    <echo level="info">Switching from instrumented to non-instrumented build: Deleting previous compilation output...</echo>
                    <delete dir="${out.classes.absolute.dir}" verbose="${verbose}" />
                </then>
            </if>
        </else>
    </if>

    <echo level="info">Resolving Build Target for ${ant.project.name}...</echo>
    <!-- load project properties, resolve Android target, library dependencies
         and set some properties with the results.
         All property names are passed as parameters ending in -Out -->
    <gettarget
            androidJarFileOut="project.target.android.jar"
            androidAidlFileOut="project.target.framework.aidl"
            bootClassPathOut="project.target.class.path"
            targetApiOut="project.target.apilevel"
            minSdkVersionOut="project.minSdkVersion" />

    <!-- 将AndroidManifest.xml文件中application节点的属性android:hasCode(表示该工程是否含有java代码，默认为true)赋值给manifest.hasCode -->
    <!-- Value of the hasCode attribute (Application node) extracted from manifest file -->
    <xpath input="${manifest.abs.file}" expression="/manifest/application/@android:hasCode"
                output="manifest.hasCode" default="true"/>

    <echo level="info">----------</echo>
    <echo level="info">Creating output directories if needed...</echo>
    <mkdir dir="${resource.absolute.dir}" />
    <mkdir dir="${jar.libs.absolute.dir}" />
    <mkdir dir="${out.absolute.dir}" />
    <mkdir dir="${out.res.absolute.dir}" />
    <mkdir dir="${out.rs.obj.absolute.dir}" />
    <mkdir dir="${out.rs.libs.absolute.dir}" />
    <!-- 如果manifest.hasCode为true -->
    <do-only-if-manifest-hasCode>
        <mkdir dir="${gen.absolute.dir}" />
        <mkdir dir="${out.classes.absolute.dir}" />
        <mkdir dir="${out.dexed.absolute.dir}" />
    </do-only-if-manifest-hasCode>

    <echo level="info">----------</echo>
    <echo level="info">Resolving Dependencies for ${ant.project.name}...</echo>
    <!-- 统计该项目的依赖关系
    libraryFolderPathOut: 存储所有依赖库工程的路径
    libraryPackagesOut: 存储所有依赖库工程的PackageName
    libraryManifestFilePathOut: 存储所有依赖库工程的Manifest文件路径
    libraryResFolderPathOut: 存储所有依赖库工程的资源文件路径，与libraryFolderPathOut顺序相反
    libraryBinAidlFolderPathOut:
    libraryRFilePathOut:
    libraryNativeFolderPathOut:
    jarLibraryPathOut:
    -->
    <dependency
            libraryFolderPathOut="project.library.folder.path"
            libraryPackagesOut="project.library.packages"
            libraryManifestFilePathOut="project.library.manifest.file.path"
            libraryResFolderPathOut="project.library.res.folder.path"
            libraryBinAidlFolderPathOut="project.library.bin.aidl.folder.path"
            libraryRFilePathOut="project.library.bin.r.file.path"
            libraryNativeFolderPathOut="project.library.native.folder.path"
            jarLibraryPathOut="project.all.jars.path"
            targetApi="${project.target.apilevel}"
            renderscriptSupportMode="${renderscript.support.mode}"
            buildToolsFolder="${android.build.tools.dir}"
            renderscriptSupportLibsOut="project.rs.support.libs.path"
            verbose="${verbose}" />

    <!-- 编译依赖库工程 -->
    <!-- compile the libraries if any -->
    <if>
        <!-- 如果定义project.library.folder.path且未定义dont.do.deps(不编译依赖库工程) -->
        <condition>
            <and>
                <isreference refid="project.library.folder.path" />
                <not><isset property="dont.do.deps" /></not>
            </and>
        </condition>
        <then>
            <!-- build.target取值范围{debug, release, instrument等} -->
            <!-- figure out which target must be used to build the library projects.
                 If emma is enabled, then use 'instrument' otherwise, use 'debug' -->
            <condition property="project.libraries.target" value="instrument" else="${build.target}">
                <istrue value="${build.is.instrumented}" />
            </condition>

            <echo level="info">----------</echo>
            <echo level="info">Building Libraries with '${project.libraries.target}'...</echo>

            <!-- ant执行依赖库工程中的build.xml -->
            <!-- no need to build the deps as we have already
                 the full list of libraries -->
            <subant failonerror="true"
                    buildpathref="project.library.folder.path"
                    antfile="build.xml">
                <!-- 既然已经编译依赖库工程，则设置dont.do.deps -->
                <target name="nodeps" />
                <!-- 执行debug/release等target -->
                <target name="${project.libraries.target}" />
                <property name="emma.coverage.absolute.file" location="${out.absolute.dir}/coverage.em" />
            </subant>
        </then>
    </if>

    <!-- compile the main project if this is a test project -->
    <if condition="${project.is.test}">
        <then>
            <!-- figure out which target must be used to build the tested project.
                 If emma is enabled, then use 'instrument' otherwise, use 'debug' -->
            <condition property="tested.project.target" value="instrument" else="debug">
                <isset property="emma.enabled" />
            </condition>

            <echo level="info">----------</echo>
            <echo level="info">Building tested project at ${tested.project.absolute.dir} with '${tested.project.target}'...</echo>
            <subant target="${tested.project.target}" failonerror="true">
                <fileset dir="${tested.project.absolute.dir}" includes="build.xml" />
            </subant>

            <!-- get the tested project full classpath to be able to build
                 the test project -->
            <testedprojectclasspath
                    projectLocation="${tested.project.absolute.dir}"
                    projectClassPathOut="tested.project.classpath"/>
        </then>
        <else>
            <!-- no tested project, make an empty Path object so that javac doesn't
                 complain -->
            <path id="tested.project.classpath" />
        </else>
    </if>
  </target>

  <!-- Code Generation: compile resources (aapt -> R.java), aidl, renderscript -->
  <target name="-code-gen">
    <!-- 合并依赖库工程中的AndroidManifest到主工程AndroidManifest中 -->
    <!-- always merge manifest -->
    <mergemanifest
            appManifest="${manifest.abs.file}"
            outManifest="${out.manifest.abs.file}"
            enabled="${manifestmerger.enabled}">
        <library refid="project.library.manifest.file.path" />
    </mergemanifest>

    <!-- 如果工程中含有java源码，执行以下指令 -->
    <do-only-if-manifest-hasCode
            elseText="hasCode = false. Skipping aidl/renderscript/R.java">
        <echo level="info">Handling aidl files...</echo>
        <!-- 编译aidl文件 -->
        <aidl executable="${aidl}"
                framework="${project.target.framework.aidl}"
                libraryBinAidlFolderPathRefid="project.library.bin.aidl.folder.path"
                genFolder="${gen.absolute.dir}"
                aidlOutFolder="${out.aidl.absolute.dir}">
            <source path="${source.absolute.dir}"/>
        </aidl>

        <!-- renderscript generates resources so it must be called before aapt -->
        <echo level="info">----------</echo>
        <echo level="info">Handling RenderScript files...</echo>
        <!-- set the rs target prop in case it hasn't been set. -->
        <property name="renderscript.target" value="${project.minSdkVersion}" />
        <renderscript
                buildToolsRoot="${android.build.tools.dir}"
                genFolder="${gen.absolute.dir}"
                resFolder="${out.res.absolute.dir}"
                rsObjFolder="${out.rs.obj.absolute.dir}"
                libsFolder="${out.rs.libs.absolute.dir}"
                targetApi="${renderscript.target}"
                optLevel="${renderscript.opt.level}"
                supportMode="${renderscript.support.mode}"
                binFolder="${out.absolute.dir}"
                buildType="${build.is.packaging.debug}"
                previousBuildType="${build.last.is.packaging.debug}">
            <source path="${source.absolute.dir}"/>
        </renderscript>

        <echo level="info">----------</echo>
        <echo level="info">Handling Resources...</echo>
        <!-- 打包Android资源文件 -->
        <aapt executable="${aapt}"
                command="package"
                verbose="${verbose}"
                manifest="${out.manifest.abs.file}"
                originalManifestPackage="${project.app.package}"
                androidjar="${project.target.android.jar}"
                rfolder="${gen.absolute.dir}"
                nonConstantId="${android.library}"
                libraryResFolderPathRefid="project.library.res.folder.path"
                libraryPackagesRefid="project.library.packages"
                libraryRFileRefid="project.library.bin.r.file.path"
                ignoreAssets="${aapt.ignore.assets}"
                binFolder="${out.absolute.dir}"
                proguardFile="${out.absolute.dir}/proguard.txt">
            <res path="${out.res.absolute.dir}" />
            <res path="${resource.absolute.dir}" />
        </aapt>

        <echo level="info">----------</echo>
        <echo level="info">Handling BuildConfig class...</echo>
        <buildconfig
                genFolder="${gen.absolute.dir}"
                package="${project.app.package}"
                buildType="${build.is.packaging.debug}"
                previousBuildType="${build.last.is.packaging.debug}"/>

    </do-only-if-manifest-hasCode>
  </target>
  <!-- empty default pre-compile target. Create a similar target in
       your build.xml and it'll be called instead of this one. -->
  <target name="-pre-compile"/>

  <!-- 编译工程，将java文件编译出class文件 -->
  <!-- Compiles this project's .java files into .class files. -->
  <target name="-compile" depends="-pre-build, -build-setup, -code-gen, -pre-compile">
    <!-- 如果工程中含有java源码，执行以下指令 -->
  	<do-only-if-manifest-hasCode elseText="hasCode = false. Skipping...">
  		<!-- merge the project's own classpath and the tested project's classpath -->
  		<path id="project.javac.classpath">
  			<path refid="project.all.jars.path" />
  			<path refid="tested.project.classpath" />
  			<path path="${java.compiler.classpath}" />
  		</path>
      <!-- 编译java文件生成class文件 -->
  		<javac encoding="${java.encoding}"
  				source="${java.source}" target="${java.target}"
  				debug="true" extdirs="" includeantruntime="false"
  				destdir="${out.classes.absolute.dir}"
  				bootclasspathref="project.target.class.path"
  				verbose="${verbose}"
  				classpathref="project.javac.classpath"
  				fork="${need.javac.fork}">
  			<src path="${source.absolute.dir}" />
  			<src path="${gen.absolute.dir}" />
  			<compilerarg line="${java.compilerargs}" />
  		</javac>
      <!-- 如果是测试工程 -->
  		<!-- if the project is instrumented, intrument the classes -->
  		<if condition="${build.is.instrumented}">
  			<then>
  				<echo level="info">Instrumenting classes from ${out.absolute.dir}/classes...</echo>

  				<!-- build the filter to remove R, Manifest, BuildConfig -->
  				<getemmafilter
  						appPackage="${project.app.package}"
  						libraryPackagesRefId="project.library.packages"
  						filterOut="emma.default.filter"/>

  				<!-- define where the .em file is going. This may have been
  					 setup already if this is a library -->
  				<property name="emma.coverage.absolute.file" location="${out.absolute.dir}/coverage.em" />

  				<!-- It only instruments class files, not any external libs -->
  				<emma enabled="true">
  					<instr verbosity="${verbosity}"
  						   mode="overwrite"
  						   instrpath="${out.absolute.dir}/classes"
  						   outdir="${out.absolute.dir}/classes"
  						   metadatafile="${emma.coverage.absolute.file}">
  						<filter excludes="${emma.default.filter}" />
  						<filter value="${emma.filter}" />
  					</instr>
  				</emma>
  			</then>
  		</if>
      <!-- 如果是library工程，则生成jar文件(名称为classes.jar) -->
  		<!-- if the project is a library then we generate a jar file -->
  		<if condition="${project.is.library}">
  			<then>
  				<echo level="info">Creating library output jar file...</echo>
  				<property name="out.library.jar.file" location="${out.absolute.dir}/classes.jar" />
  				<if>
            <!-- android.package.excludes(默认空字符串):针对src目录设定的过滤规则，避免某些文件被打包进最终的APK中 -->
  					<condition>
  						<length string="${android.package.excludes}" trim="true" when="greater" length="0" />
  					</condition>
  					<then>
  						<echo level="info">Custom jar packaging exclusion: ${android.package.excludes}</echo>
  					</then>
  				</if>

  				<propertybyreplace name="project.app.package.path" input="${project.app.package}" replace="." with="/" />
          <!-- 打jar包 -->
  				<jar destfile="${out.library.jar.file}">
  					<fileset dir="${out.classes.absolute.dir}"
  							includes="**/*.class"
  							excludes="${project.app.package.path}/R.class ${project.app.package.path}/R$*.class ${project.app.package.path}/BuildConfig.class"/>
  					<fileset dir="${source.absolute.dir}" excludes="**/*.java ${android.package.excludes}" />
  				</jar>
  			</then>
  		</if>

  	</do-only-if-manifest-hasCode>
  </target>
  <!-- empty default post-compile target. Create a similar target in
         your build.xml and it'll be called instead of this one. -->
  <target name="-post-compile"/>

  <!-- 执行代码混淆 -->
  <!-- Obfuscate target
      This is only active in release builds when proguard.config is defined
      in default.properties.

      To replace Proguard with a different obfuscation engine:
      Override the following targets in your build.xml, before the call to <setup>
          -release-obfuscation-check
              Check whether obfuscation should happen, and put the result in a property.
          -debug-obfuscation-check
              Obfuscation should not happen. Set the same property to false.
          -obfuscate
              check if the property set in -debug/release-obfuscation-check is set to true.
              If true:
                  Perform obfuscation
                  Set property out.dex.input.absolute.dir to be the output of the obfuscation
  -->
  <target name="-obfuscate">
      <!-- 如果开启代码混淆 -->
      <if condition="${proguard.enabled}">
          <then>
              <property name="obfuscate.absolute.dir" location="${out.absolute.dir}/proguard" />
              <property name="preobfuscate.jar.file" value="${obfuscate.absolute.dir}/original.jar" />
              <property name="obfuscated.jar.file" value="${obfuscate.absolute.dir}/obfuscated.jar" />
              <!-- input for dex will be proguard's output -->
              <property name="out.dex.input.absolute.dir" value="${obfuscated.jar.file}" />

              <!-- Add Proguard Tasks -->
              <property name="proguard.jar" location="${android.tools.dir}/proguard/lib/proguard.jar" />
              <taskdef name="proguard" classname="proguard.ant.ProGuardTask" classpath="${proguard.jar}" />

              <!-- Set the android classpath Path object into a single property. It'll be
                   all the jar files separated by a platform path-separator.
                   Each path must be quoted if it contains spaces.
              -->
              <pathconvert property="project.target.classpath.value" refid="project.target.class.path">
                  <firstmatchmapper>
                      <regexpmapper from='^([^ ]*)( .*)$$' to='"\1\2"'/>
                      <identitymapper/>
                  </firstmatchmapper>
              </pathconvert>

              <!-- Build a path object with all the jar files that must be obfuscated.
                   This include the project compiled source code and any 3rd party jar
                   files. -->
              <path id="project.all.classes.path">
                  <pathelement location="${preobfuscate.jar.file}" />
                  <path refid="project.all.jars.path" />
              </path>
              <!-- Set the project jar files Path object into a single property. It'll be
                   all the jar files separated by a platform path-separator.
                   Each path must be quoted if it contains spaces.
              -->
              <pathconvert property="project.all.classes.value" refid="project.all.classes.path">
                  <firstmatchmapper>
                      <regexpmapper from='^([^ ]*)( .*)$$' to='"\1\2"'/>
                      <identitymapper/>
                  </firstmatchmapper>
              </pathconvert>

              <!-- Turn the path property ${proguard.config} from an A:B:C property
                   into a series of includes: -include A -include B -include C
                   suitable for processing by the ProGuard task. Note - this does
                   not include the leading '-include "' or the closing '"'; those
                   are added under the <proguard> call below.
              -->
              <path id="proguard.configpath">
                  <pathelement path="${proguard.config}"/>
              </path>
              <pathconvert pathsep='" -include "' property="proguard.configcmd" refid="proguard.configpath"/>

              <mkdir   dir="${obfuscate.absolute.dir}" />
              <delete file="${preobfuscate.jar.file}"/>
              <delete file="${obfuscated.jar.file}"/>
              <jar basedir="${out.classes.absolute.dir}"
                  destfile="${preobfuscate.jar.file}" />
              <proguard>
                  -include      "${proguard.configcmd}"
                  -include      "${out.absolute.dir}/proguard.txt"
                  -injars       ${project.all.classes.value}
                  -outjars      "${obfuscated.jar.file}"
                  -libraryjars  ${project.target.classpath.value}
                  -dump         "${obfuscate.absolute.dir}/dump.txt"
                  -printseeds   "${obfuscate.absolute.dir}/seeds.txt"
                  -printusage   "${obfuscate.absolute.dir}/usage.txt"
                  -printmapping "${obfuscate.absolute.dir}/mapping.txt"
              </proguard>
          </then>
      </if>
  </target>

  <!-- 将class文件转换成dex文件 -->
  <!-- Converts this project's .class files into .dex files -->
  <target name="-dex" depends="-compile, -post-compile, -obfuscate">
  	<do-only-if-manifest-hasCode elseText="hasCode = false. Skipping...">
      <!-- 如果不是library工程 -->
  		<!-- only convert to dalvik bytecode is *not* a library -->
  		<do-only-if-not-library elseText="Library project: do not convert bytecode..." >
  			<!-- special case for instrumented builds: need to use no-locals and need
  				 to pass in the emma jar. -->
  			<if condition="${build.is.instrumented}">
  				<then>
  					<dex-helper nolocals="true">
  						<external-libs>
  							<fileset file="${emma.dir}/emma_device.jar" />
  						</external-libs>
  					</dex-helper>
  				</then>
  				<else>
            <!-- 执行:dex-helper -->
  					<dex-helper />
  				</else>
  			</if>
  		</do-only-if-not-library>
  	</do-only-if-manifest-hasCode>
  </target>

  <!-- 预处理png文件 -->
  <!-- Updates the pre-processed PNG cache -->
  <target name="-crunch">
      <exec executable="${aapt}" taskName="crunch">
          <arg value="crunch" />
          <arg value="-v" />
          <arg value="-S" />
          <arg path="${resource.absolute.dir}" />
          <arg value="-C" />
          <arg path="${out.res.absolute.dir}" />
      </exec>
  </target>

  <!-- 打包资源文件 -->
  <!-- Puts the project's resources into the output package file
       This actually can create multiple resource package in case
       Some custom apk with specific configuration have been
       declared in default.properties.
       -->
  <target name="-package-resources" depends="-crunch">
      <!-- only package resources if *not* a library project -->
      <do-only-if-not-library elseText="Library project: do not package resources..." >
          <aapt executable="${aapt}"
                  command="package"
                  versioncode="${version.code}"
                  versionname="${version.name}"
                  debug="${build.is.packaging.debug}"
                  manifest="${out.manifest.abs.file}"
                  assets="${asset.absolute.dir}"
                  androidjar="${project.target.android.jar}"
                  apkfolder="${out.absolute.dir}"
                  nocrunch="${build.packaging.nocrunch}"
                  resourcefilename="${resource.package.file.name}"
                  resourcefilter="${aapt.resource.filter}"
                  libraryResFolderPathRefid="project.library.res.folder.path"
                  libraryPackagesRefid="project.library.packages"
                  libraryRFileRefid="project.library.bin.r.file.path"
                  previousBuildType="${build.last.target}"
                  buildType="${build.target}"
                  ignoreAssets="${aapt.ignore.assets}">
              <res path="${out.res.absolute.dir}" />
              <res path="${resource.absolute.dir}" />
              <!-- <nocompress /> forces no compression on any files in assets or res/raw -->
              <!-- <nocompress extension="xml" /> forces no compression on specific file extensions in assets and res/raw -->
          </aapt>
      </do-only-if-not-library>
  </target>

  <!-- 打包各种资源生成apk文件(*.so库文件也是在此打包的) -->
  <!-- Packages the application. -->
  <target name="-package" depends="-dex, -package-resources">
  	<!-- only package apk if *not* a library project -->
  	<do-only-if-not-library elseText="Library project: do not package apk..." >
  		<if condition="${build.is.instrumented}">
  			<then>
  				<package-helper>
  					<extra-jars>
  						<!-- Injected from external file -->
  						<jarfile path="${emma.dir}/emma_device.jar" />
  					</extra-jars>
  				</package-helper>
  			</then>
  			<else>
  				<package-helper />
  			</else>
  		</if>
  	</do-only-if-not-library>
  </target>

  <target name="-post-package" />

  <!-- 提示输入签名文件信息 -->
  <!-- called through target 'release'. Only executed if the keystore and
       key alias are known but not their password. -->
  <target name="-release-prompt-for-password" if="has.keystore" unless="has.password">
      <!-- Gets passwords -->
      <input
              message="Please enter keystore password (store:${key.store}):"
              addproperty="key.store.password" />
      <input
              message="Please enter password for alias '${key.alias}':"
              addproperty="key.alias.password" />
  </target>

  <!-- called through target 'release'. Only executed if there's no
       keystore/key alias set -->
  <target name="-release-nosign" unless="has.keystore">
      <!-- no release builds for library project -->
      <do-only-if-not-library elseText="" >
          <sequential>
              <echo level="info">No key.store and key.alias properties found in build.properties.</echo>
              <echo level="info">Please sign ${out.packaged.file} manually</echo>
              <echo level="info">and run zipalign from the Android SDK tools.</echo>
          </sequential>
      </do-only-if-not-library>
      <record-build-info />
  </target>

  <!-- 签名 -->
  <target name="-release-sign" if="has.keystore" >
      <!-- only create apk if *not* a library project -->
      <do-only-if-not-library elseText="Library project: do not create apk..." >
          <sequential>
              <property name="out.unaligned.file" location="${out.absolute.dir}/${ant.project.name}-release-unaligned.apk" />

              <!-- Signs the APK -->
              <echo level="info">Signing final apk...</echo>
              <signapk
                      input="${out.packaged.file}"
                      output="${out.unaligned.file}"
                      keystore="${key.store}"
                      storepass="${key.store.password}"
                      alias="${key.alias}"
                      keypass="${key.alias.password}"/>

              <!-- Zip aligns the APK -->
              <zipalign-helper
                      in.package="${out.unaligned.file}"
                      out.package="${out.final.file}" />
              <echo level="info">Release Package: ${out.final.file}</echo>
          </sequential>
      </do-only-if-not-library>
      <record-build-info />
  </target>

  <target name="-post-build" />
</target>

<!-- Configurable macro, which allows to pass as parameters output directory,
     output dex filename and external libraries to dex (optional) -->
<macrodef name="dex-helper">
    <element name="external-libs" optional="yes" />
    <attribute name="nolocals" default="false" />
    <sequential>
        <!-- sets the primary input for dex. If a pre-dex task sets it to
             something else this has no effect -->
        <property name="out.dex.input.absolute.dir" value="${out.classes.absolute.dir}" />

        <!-- set the secondary dx input: the project (and library) jar files
             If a pre-dex task sets it to something else this has no effect -->
        <if>
            <condition>
                <isreference refid="out.dex.jar.input.ref" />
            </condition>
            <else>
                <path id="out.dex.jar.input.ref">
                    <path refid="project.all.jars.path" />
                </path>
            </else>
        </if>
        <!-- 将jar文件转换为dex文件 -->
        <dex executable="${dx}"
                output="${intermediate.dex.file}"
                dexedlibs="${out.dexed.absolute.dir}"
                nolocals="@{nolocals}"
                forceJumbo="${dex.force.jumbo}"
                disableDexMerger="${dex.disable.merger}"
                verbose="${verbose}">
            <path path="${out.dex.input.absolute.dir}"/>
            <path refid="out.dex.jar.input.ref" />
            <external-libs />
        </dex>
    </sequential>
</macrodef>

<!-- This is macro that enable passing variable list of external jar files to ApkBuilder
     Example of use:
     <package-helper>
         <extra-jars>
            <jarfolder path="my_jars" />
            <jarfile path="foo/bar.jar" />
            <jarfolder path="your_jars" />
         </extra-jars>
     </package-helper> -->
<macrodef name="package-helper">
    <element name="extra-jars" optional="yes" />
    <sequential>
        <!-- 打包各种资源生成apk文件 -->
        <apkbuilder
                outfolder="${out.absolute.dir}"
                resourcefile="${resource.package.file.name}"
                apkfilepath="${out.packaged.file}"
                debugpackaging="${build.is.packaging.debug}"
                debugsigning="${build.is.signing.debug}"
                verbose="${verbose}"
                hascode="${manifest.hasCode}"
                previousBuildType="${build.last.is.packaging.debug}/${build.last.is.signing.debug}"
                buildType="${build.is.packaging.debug}/${build.is.signing.debug}">
            <dex path="${intermediate.dex.file}"/>
            <sourcefolder path="${source.absolute.dir}"/>
            <jarfile refid="project.all.jars.path" />
            <nativefolder path="${native.libs.absolute.dir}" />
            <nativefolder refid="project.library.native.folder.path" />
            <nativefolder refid="project.rs.support.libs.path" />
            <nativefolder path="${out.rs.libs.absolute.dir}" />
            <extra-jars/>
        </apkbuilder>
    </sequential>
</macrodef>

```


  [1]: http://ant.apache.org/bindownload.cgi