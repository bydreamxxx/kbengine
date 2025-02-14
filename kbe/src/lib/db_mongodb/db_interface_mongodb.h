#pragma once
#include "common.h"
#include "db_transaction.h"
#include "common/common.h"
#include "common/singleton.h"
#include "common/memorystream.h"
#include "helper/debug_helper.h"
#include "db_interface/db_interface.h"
#include "entitydef/entitydef.h"

#ifdef _MSC_VER //解决mongodb和python中ssize_t冲突定义的问题
#define _SSIZE_T_DEFINED
#endif
#include "mongoc.h"
#include <bson.h>
#include <bcon.h>

namespace KBEngine 
{
	class DBInterfaceMongodb : public DBInterface
	{
	public:
		DBInterfaceMongodb(const char* name);
		virtual ~DBInterfaceMongodb();

		static bool initInterface(DBInterface* pdbi);

		/**
		与某个数据库关联
		*/
		bool reattach();
		virtual bool attach(const char* databaseName = NULL);
		virtual bool detach();

		/*bool ping(){
			return mysql_ping(pMysql_) == 0;
		}*/

		bool ping(mongoc_client_t* pMongoClient = NULL);

		void inTransaction(bool value)
		{
			KBE_ASSERT(inTransaction_ != value);
			inTransaction_ = value;
		}

		bool hasLostConnection() const		{ return hasLostConnection_; }
		void hasLostConnection(bool v)	{ hasLostConnection_ = v; }

		/**
		检查环境
		*/
		virtual bool checkEnvironment();

		/**
		检查错误， 对错误的内容进行纠正
		如果纠正不成功返回失败
		*/
		virtual bool checkErrors();

		virtual bool query(const char* strCommand, uint32 size, bool printlog = true, MemoryStream * result = NULL);
		bool executeFindCommand(MemoryStream * result, std::vector<std::string> strcmd, const char *tableName);
		bool executeUpdateCommand(std::vector<std::string> strcmd, const char *tableName);
		bool executeRemoveCommand(std::vector<std::string> strcmd, const char *tableName);
		bool executeInsertCommand(std::vector<std::string> strcmd, const char *tableName);
		bool executeFunctionCommand(MemoryStream * result,std::string strcmd);
		bool extuteFunction(const bson_t *command, const mongoc_read_prefs_t *read_prefs, bson_t *reply);
		std::vector<std::string> splitParameter(std::string value);

		bool write_query_result(MemoryStream * result, const char* strcmd = NULL);

		/**
		获取数据库所有的表名
		*/
		virtual bool getTableNames(std::vector<std::string>& tableNames, const char * pattern);

		/**
		获取数据库某个表所有的字段名称
		*/
		virtual bool getTableItemNames(const char* tableName, std::vector<std::string>& itemNames);

		/*const char* getLastError()
		{
		if (pMongoClient == NULL)
		return "pMongoClient is NULL";

		return pMongoClient->in_exhaust;
		}*/

		/**
		返回这个接口的描述
		*/
		virtual const char* c_str();

		/**
		获取错误
		*/
		virtual const char* getstrerror();

		/**
		获取错误编号
		*/
		virtual int getlasterror();

		/**
		如果数据库不存在则创建一个数据库
		*/
		virtual bool createDatabaseIfNotExist();//有待验证是否需要

		/**
		创建一个entity存储表
		*/
		virtual EntityTable* createEntityTable(EntityTables* pEntityTables);

		/**
		从数据库删除entity表
		*/
		virtual bool dropEntityTableFromDB(const char* tableName);

		/**
		从数据库删除entity表字段
		*/
		virtual bool dropEntityTableItemFromDB(const char* tableName, const char* tableItemName);

		mongoc_client_t* mongo(){ return _pMongoClient; }

		/**
		锁住接口操作
		*/
		virtual bool lock();
		virtual bool unlock();

		void throwError();

		/**
		处理异常
		*/
		bool isLostConnection(std::exception & e);
		bool processException(std::exception & e);

		/**
		执行与数据库相关的操作
		*/
		bool createCollection(const char *tableName);

		bool insertCollection(const char *tableName, mongoc_insert_flags_t flags, const bson_t *document, const mongoc_write_concern_t *write_concern);

		mongoc_cursor_t *  collectionFind(const char *tableName, mongoc_query_flags_t flags, uint32_t skip, uint32_t limit, uint32_t  batch_size, const bson_t *query, const bson_t *fields, const mongoc_read_prefs_t *read_prefs);

		bool updateCollection(const char *tableName, mongoc_update_flags_t uflags, const bson_t *selector, const bson_t *update, const mongoc_write_concern_t *write_concern);

		bool collectionRemove(const char *tableName, mongoc_remove_flags_t flags, const bson_t *selector, const mongoc_write_concern_t *write_concern);

		mongoc_cursor_t * collectionFindIndexes(const char *tableName);

		bool collectionCreateIndex(const char *tableName, const bson_t *keys, const mongoc_index_opt_t *opt);

		bool collectionDropIndex(const char *tableName, const char *index_name);		

	protected:
		mongoc_client_t *_pMongoClient;
		mongoc_database_t *database;
		bool hasLostConnection_;
		bool inTransaction_;
		mongodb::DBTransaction lock_;
		const char *strError;
	};
}