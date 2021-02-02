<?xml version='1.0' encoding='UTF-8'?>
<Project Type="Project" LVVersion="19008000">
	<Item Name="My Computer" Type="My Computer">
		<Property Name="server.app.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="server.control.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="server.tcp.enabled" Type="Bool">false</Property>
		<Property Name="server.tcp.port" Type="Int">0</Property>
		<Property Name="server.tcp.serviceName" Type="Str">My Computer/VI Server</Property>
		<Property Name="server.tcp.serviceName.default" Type="Str">My Computer/VI Server</Property>
		<Property Name="server.vi.callsEnabled" Type="Bool">true</Property>
		<Property Name="server.vi.propertiesEnabled" Type="Bool">true</Property>
		<Property Name="specify.custom.address" Type="Bool">false</Property>
		<Item Name="subvis" Type="Folder">
			<Item Name="AllocateConvertFrame.vi" Type="VI" URL="../subvis/AllocateConvertFrame.vi"/>
			<Item Name="AllocateQuequeFrames.vi" Type="VI" URL="../subvis/AllocateQuequeFrames.vi"/>
			<Item Name="DeviceDiscovery.vi" Type="VI" URL="../subvis/DeviceDiscovery.vi"/>
			<Item Name="DisplayImage.vi" Type="VI" URL="../subvis/DisplayImage.vi"/>
			<Item Name="FrameSave.vi" Type="VI" URL="../subvis/FrameSave.vi"/>
			<Item Name="local_variable.vi" Type="VI" URL="../subvis/local_variable.vi"/>
			<Item Name="QueueFrame.vi" Type="VI" URL="../subvis/QueueFrame.vi"/>
			<Item Name="ReleaseFrames.vi" Type="VI" URL="../subvis/ReleaseFrames.vi"/>
			<Item Name="ResetKnob.vi" Type="VI" URL="../subvis/ResetKnob.vi"/>
			<Item Name="ResetSlide.vi" Type="VI" URL="../subvis/ResetSlide.vi"/>
			<Item Name="Streaming.vi" Type="VI" URL="../subvis/Streaming.vi"/>
			<Item Name="test.vi" Type="VI" URL="../subvis/test.vi"/>
			<Item Name="test1.vi" Type="VI" URL="../subvis/test1.vi"/>
		</Item>
		<Item Name="eCapture.vi" Type="VI" URL="../eCapture.vi"/>
		<Item Name="EmergentCameraC.lvlib" Type="Library" URL="../EmergentCameraC/EmergentCameraC.lvlib"/>
		<Item Name="Dependencies" Type="Dependencies">
			<Item Name="vi.lib" Type="Folder">
				<Item Name="Acquire Semaphore.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/semaphor.llb/Acquire Semaphore.vi"/>
				<Item Name="AddNamedSemaphorePrefix.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/semaphor.llb/AddNamedSemaphorePrefix.vi"/>
				<Item Name="datatype.ctl" Type="VI" URL="/&lt;vilib&gt;/Utility/importsl/GetValueByPointer/datatype.ctl"/>
				<Item Name="DSDisposePtr.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/importsl/DSDisposePtr.vi"/>
				<Item Name="DSNewPtr.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/importsl/DSNewPtr.vi"/>
				<Item Name="Error Cluster From Error Code.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/error.llb/Error Cluster From Error Code.vi"/>
				<Item Name="GetNamedSemaphorePrefix.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/semaphor.llb/GetNamedSemaphorePrefix.vi"/>
				<Item Name="Image Type" Type="VI" URL="/&lt;vilib&gt;/vision/Image Controls.llb/Image Type"/>
				<Item Name="IMAQ Create" Type="VI" URL="/&lt;vilib&gt;/vision/Basics.llb/IMAQ Create"/>
				<Item Name="IMAQ GetImagePixelPtr" Type="VI" URL="/&lt;vilib&gt;/vision/Basics.llb/IMAQ GetImagePixelPtr"/>
				<Item Name="IMAQ Image.ctl" Type="VI" URL="/&lt;vilib&gt;/vision/Image Controls.llb/IMAQ Image.ctl"/>
				<Item Name="Not A Semaphore.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/semaphor.llb/Not A Semaphore.vi"/>
				<Item Name="Obtain Semaphore Reference.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/semaphor.llb/Obtain Semaphore Reference.vi"/>
				<Item Name="Release Semaphore.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/semaphor.llb/Release Semaphore.vi"/>
				<Item Name="Semaphore RefNum" Type="VI" URL="/&lt;vilib&gt;/Utility/semaphor.llb/Semaphore RefNum"/>
				<Item Name="Semaphore Refnum Core.ctl" Type="VI" URL="/&lt;vilib&gt;/Utility/semaphor.llb/Semaphore Refnum Core.ctl"/>
				<Item Name="Validate Semaphore Size.vi" Type="VI" URL="/&lt;vilib&gt;/Utility/semaphor.llb/Validate Semaphore Size.vi"/>
			</Item>
			<Item Name="nivissvc.dll" Type="Document" URL="nivissvc.dll">
				<Property Name="NI.PreserveRelativePath" Type="Bool">true</Property>
			</Item>
		</Item>
		<Item Name="Build Specifications" Type="Build">
			<Item Name="test" Type="EXE">
				<Property Name="App_copyErrors" Type="Bool">true</Property>
				<Property Name="App_INI_aliasGUID" Type="Str">{E59C4FFA-B127-4340-BFF7-AAB5AC09B1A7}</Property>
				<Property Name="App_INI_GUID" Type="Str">{D45A77FF-190B-4A49-88CC-C72A3BD259C1}</Property>
				<Property Name="App_serverConfig.httpPort" Type="Int">8002</Property>
				<Property Name="Bld_autoIncrement" Type="Bool">true</Property>
				<Property Name="Bld_buildCacheID" Type="Str">{C0677F89-352D-42BD-942A-F55AC01522A0}</Property>
				<Property Name="Bld_buildSpecName" Type="Str">test</Property>
				<Property Name="Bld_excludeInlineSubVIs" Type="Bool">true</Property>
				<Property Name="Bld_excludeLibraryItems" Type="Bool">true</Property>
				<Property Name="Bld_excludePolymorphicVIs" Type="Bool">true</Property>
				<Property Name="Bld_localDestDir" Type="Path">../builds/NI_AB_PROJECTNAME/test</Property>
				<Property Name="Bld_localDestDirType" Type="Str">relativeToCommon</Property>
				<Property Name="Bld_modifyLibraryFile" Type="Bool">true</Property>
				<Property Name="Bld_previewCacheID" Type="Str">{9A78DC9F-D544-4725-8A9F-C71B86DAB35F}</Property>
				<Property Name="Bld_version.major" Type="Int">1</Property>
				<Property Name="Destination[0].destName" Type="Str">Application.exe</Property>
				<Property Name="Destination[0].path" Type="Path">../builds/NI_AB_PROJECTNAME/test/Application.exe</Property>
				<Property Name="Destination[0].preserveHierarchy" Type="Bool">true</Property>
				<Property Name="Destination[0].type" Type="Str">App</Property>
				<Property Name="Destination[1].destName" Type="Str">Support Directory</Property>
				<Property Name="Destination[1].path" Type="Path">../builds/NI_AB_PROJECTNAME/test/data</Property>
				<Property Name="DestinationCount" Type="Int">2</Property>
				<Property Name="Source[0].itemID" Type="Str">{461F2BC4-08ED-4F87-BE47-89135C02C7FE}</Property>
				<Property Name="Source[0].type" Type="Str">Container</Property>
				<Property Name="Source[1].destinationIndex" Type="Int">0</Property>
				<Property Name="Source[1].itemID" Type="Ref">/My Computer/eCapture.vi</Property>
				<Property Name="Source[1].sourceInclusion" Type="Str">TopLevel</Property>
				<Property Name="Source[1].type" Type="Str">VI</Property>
				<Property Name="SourceCount" Type="Int">2</Property>
				<Property Name="TgtF_fileDescription" Type="Str">test</Property>
				<Property Name="TgtF_internalName" Type="Str">test</Property>
				<Property Name="TgtF_legalCopyright" Type="Str">Copyright ?2014 </Property>
				<Property Name="TgtF_productName" Type="Str">test</Property>
				<Property Name="TgtF_targetfileGUID" Type="Str">{622811D0-62AB-421E-9287-68B570765BE6}</Property>
				<Property Name="TgtF_targetfileName" Type="Str">Application.exe</Property>
			</Item>
		</Item>
	</Item>
</Project>
