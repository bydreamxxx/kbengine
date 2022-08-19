# KBEngine for kst

这是 KBEngine 的 kst 分支，用于开发大型 MMORPG 游戏。根据实际的项目开发需要对 KBEngine 的特性进行扩展，并依托项目对 KBEngine 进行有针对性的深度压力测试，通过压测，得到KBEngine服务器的性能极限数据，得到了与运营有关的服务器优化配置参数，发现并修正了多项在正常情况下难以发现或重现的幽灵bug。 

## 扩展功能如下：  
 - 支持 ue4 引擎导出的导航数据，扩展寻路导航功能  
支持多层导航数据和导航标志参数，除了让怪物在服务器中能正确的寻路外，还能根据需要控制寻路的区域——例如桥被炸后就会绕着走或无法导航到另一端。  

- 増加交通工具的机制，为飞船载人飞行等玩法提供基础支持  

- cellapp 増加了与具体地图无关的导航数据加载和导航路径计算接口，为服务器提供在飞行器上寻路的基础  

- 增加了用于在飞行器上动态寻路的动态寻路技术  

- 完善了 Entity.controlledBy 机制，使之与 bigworld 的用法完全一致  

- 为 dbmgr 増加了mongodb 的支持  

- cluster_controller.py 功能扩展和完善  

- Telnet console 功能扩展和完善  

- 加入对跨服玩法的原生功能支持，新增组件 centermgr

- 支持KCP协议

--- 
KBEngine
========

[![Build Status](https://travis-ci.org/kbengine/kbengine.svg)](https://travis-ci.org/kbengine/kbengine)
[![Appveyor (Windows) Build  Status](https://ci.appveyor.com/api/projects/status/github/kbengine/kbengine?branch=master&svg=true)](https://ci.appveyor.com/project/kbengine/kbengine/branch/master)


## Homepage

	http://kbengine.org


## Releases

	Sources		: https://github.com/kbengine/kbengine/releases/latest


## Demo sources

	Unity3d		: https://github.com/kbengine/kbengine_unity3d_demo/releases/latest
	Unity3d		: https://github.com/kbengine/kbengine_unity3d_warring/releases/latest
	UE4		: https://github.com/kbengine/kbengine_ue4_demo/releases/latest
	Ogre		: https://github.com/kbengine/kbengine_ogre_demo/releases/latest
	Cocos2d_js	: https://github.com/kbengine/kbengine_cocos2d_js_demo/releases/latest


## Docs

	Docs		: http://kbengine.github.io/docs/
	API		: https://github.com/kbengine/kbengine/tree/master/docs


## Support

	Email		: kbesrv?gmail.com
	QQ		: 3603661
	BBS		: http://bbs.kbengine.org
	Maillist	: https://groups.google.com/d/forum/kbengine_maillist

## Stresstest

	https://www.youtube.com/watch?v=sWtk3CfxyGY
	http://v.youku.com/v_show/id_XMjgyMjM0MTYwNA==.html?spm=a2h3j.8428770.3416059.1


## What is KBEngine?

	An open source MMOG server engine. 
	Just use Python scripting to be able to complete any game logic simply and efficiently (supports hotfixing).
	Various KBEngine plugins can be quickly combined with (Unity3D, OGRE, Cocos2d-x, HTML5, etc.) technology to 
	form a complete game client.

	The engine is written in C++, and saves developers from having to re-implement common server-side 
	technology, allowing them to concentrate on game logic development, to quickly create a variety of games.

	(Because it is often asked what the upper limit of the load is that KBEngine can handle, the underlying 
	architecture has been designed as a multi-process distributed dynamic load balancing solution. In theory, 
	by continuously expanding the hardware, the upper limit of the load can also be continuously increased. 
	The upper limit of the capacity of a single machine depends on the complexity of the game logic itself.)


## Create a new game asset library

	Execute:
		new_assets.bat

	Output:
		server_assets


## 中文

[官网](http://kbengine.github.io/cn)，[论坛](http://bbs.kbengine.org)，QQ交流群：837764110、461368412、16535321


## 什么是KBEngine?

	一款开源的MMOG游戏服务端引擎，
	仅Python脚本即可简单高效的完成任何游戏逻辑(支持热更新)，
	使用配套客户端插件能够快速与（Unity3D、UE4、OGRE、HTML5、等等）结合形成一个完整的客户端。

	引擎使用C++编写，开发者无需重复的实现游戏服务端通用的底层技术，
	将精力真正集中到游戏开发层面上来，稳定可靠并且快速的打造各种网络游戏。

	(经常被问到承载上限，KBEngine底层架构被设计为多进程分布式动态负载均衡方案，
	理论上只需要不断扩展硬件就能够不断增加承载上限，单台机器的承载上限取决于游戏逻辑本身的复杂度。)


