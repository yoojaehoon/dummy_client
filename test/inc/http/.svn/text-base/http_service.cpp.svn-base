
int send_page(MHD_Connection* connection, const char* page, int status_code)
{
    MHD_Response* response;

//	printf("send_page data : %s\nlength:%d\n", page, (int)strlen(page));

    response = MHD_create_response_from_data(strlen(page), (void*)page, MHD_NO, MHD_YES);

	if (!response)
        return MHD_NO;
	
	MHD_add_response_header (response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json; charset=UTF-8");
	
    int ret = MHD_queue_response(connection, status_code, response);
    MHD_destroy_response(response);
    return ret;
}

// 처리
int Work_function(MHD_Connection* connection, SConnectInfo* psConnInfo)
{
	CCloudMSApi clsMSApi;

	// Socket 가져오기
	int client_sockfd = 0;
	struct MHD_Pollfd p;
    if( MHD_connection_get_pollfd(connection, &p) == MHD_YES )
		client_sockfd = p.fd;

	clsMSApi.Init(psConnInfo->connection_type
		, psConnInfo->url.c_str()
		, psConnInfo->header.c_str()
		, psConnInfo->query.c_str()
		, psConnInfo->body.c_str()
		, client_sockfd
	);

	if( !clsMSApi.StartWork() )	// 실패
	{
		CMiscUtil::WriteErrorLog("StartWork FAILED", client_sockfd, 0);
		return send_page(connection, "", clsMSApi.m_responsecode);
	}

	char* strBody = NULL;
	int body_size = clsMSApi.GetResponseBodySize();

	CMiscUtil::WriteLog("GetResponseBodySize()", 0, 1, &body_size);

	if(body_size > 0 )
	{
		strBody = new char[body_size];

		if( !clsMSApi.GetResponseBody(strBody, body_size) )
		{
			if( !strBody )
				delete [] strBody;

			CMiscUtil::WriteErrorLog("Work_function GetResponseBody FAILED", client_sockfd, 0);
			return send_page(connection, "", clsMSApi.m_responsecode);
		}

		int ret = send_page(connection, strBody, clsMSApi.m_responsecode);

		if( !strBody )
			delete [] strBody;

		return ret;
	}
	else
	{
		return send_page(connection, "", clsMSApi.m_responsecode); 
	}
}

int callback_client_connect(void* cls, const sockaddr* addr, socklen_t addrlen)
{
	//! \todo Here you can do any statistics, IP filtering, etc.
	//char buf[INET6_ADDRSTRLEN];
	//printf("Incoming IP: %s\n", inet_ntop(addr->sa_family, addr->sa_data + 2, buf, INET6_ADDRSTRLEN));
	//! \todo Return MHD_NO if you want to refuse the connection.
	return MHD_YES;
}

// Header 정보
int iterate_header(void *strHeader, enum MHD_ValueKind kind, const char *key, const char *value)
{
	char *data = (char *)strHeader;

	if( strcmp(data, "") == 0 )
	{
		sprintf(data, "{ \"%s\" : \"%s\" ", key, value);
	}
	else
	{
		sprintf(data, "%s, \"%s\" : \"%s\" ", data, key, value);	
	}

	return MHD_YES;
}

// Get 정보
int iterate_getdata(void *strdata, enum MHD_ValueKind kind, const char *key, const char *value)
{
	char *data = (char *)strdata;

	if( strcmp(data, "") == 0 )
	{
		sprintf(data, "{ \"%s\" : \"%s\" ", key, value);
	}
	else
	{
		sprintf(data, "%s, \"%s\" : \"%s\" ", data, key, value);	
	}

	return MHD_YES;
}

int callback_request_to_connect(void* cls, MHD_Connection* connection,
                         const char* url, const char* method,
                         const char* version, const char* upload_data,
                         size_t* upload_data_size, void** con_cls)
{
	SConnectInfo* con_info = NULL;
	
    if (NULL == *con_cls)    // Incoming!... prepare all.
    {
    	con_info = new SConnectInfo;
        if (NULL == con_info)
            return MHD_NO;

		if (0 == strcmp(method, "GET"))
            con_info->connection_type = GET;
        else if (0 == strcmp(method, "POST"))
            con_info->connection_type = POST;
        else if (0 == strcmp(method, "PUT"))
			con_info->connection_type = PUT;
		else if (0 == strcmp(method, "DELETE"))
			con_info->connection_type = DELETE;
        else
            return MHD_NO;

        *con_cls = (void*) con_info;

        return MHD_YES;
    }

	con_info = (SConnectInfo*) *con_cls;

	if( con_info->connection_type == POST || con_info->connection_type == PUT )
	{
		// Body 가져오기
		if (0 != *upload_data_size)        // chunk of POST data?
        {
            // we're still receiving the POST data, process them.
            MHD_post_process(con_info->post_processor, upload_data, *upload_data_size);

            con_info->body += upload_data;

            *upload_data_size = 0;    // 0 means "successfully processed"
            return MHD_YES;
        }
        else    // awaiting reply
        {
//			printf("data - %s\n", (con_info->body).c_str());

			// URL //////////////////////////////////////////////////////
			con_info->url = url;
//			printf("URL - %s\n", (con_info->url).c_str());

			// Header 정보 ///////////////////////////////////////////////
			char strHeader[20*1024];
			memset(strHeader, 0x00, sizeof(strHeader));
			MHD_get_connection_values(connection, MHD_HEADER_KIND, iterate_header, (void*)strHeader);
			
			if( strcmp(strHeader, "") != 0 )
				sprintf(strHeader, "%s }", strHeader);

			con_info->header = strHeader;

            return Work_function(connection, con_info);
        }
	}
	
	if( con_info->connection_type == GET || con_info->connection_type == DELETE )
	{
		char strGetData[512];
		memset(strGetData, 0x00, sizeof(strGetData));
		MHD_get_connection_values(connection, MHD_GET_ARGUMENT_KIND, iterate_getdata, (void*)strGetData);

		if( strcmp(strGetData, "") != 0 )
			sprintf(strGetData, "%s }", strGetData);

		con_info->query = strGetData;

		//printf("URL Query - %s\n", (con_info->query).c_str());

		// URL //////////////////////////////////////////////////////
		con_info->url = url;
		//printf("URL - %s\n", (con_info->url).c_str());

		// Header 정보 ///////////////////////////////////////////////
		char strHeader[512];
		memset(strHeader, 0x00, sizeof(strHeader));
		MHD_get_connection_values(connection, MHD_HEADER_KIND, iterate_header, (void*)strHeader);

		if( strcmp(strHeader, "") != 0 )
			sprintf(strHeader, "%s }", strHeader);

		con_info->header = strHeader;

		return Work_function(connection, con_info);
	}
	else
		return MHD_NO;

	return MHD_YES;
}

void callback_request_completed(void* cls, MHD_Connection* connection,
                       void** con_cls, MHD_RequestTerminationCode toe)
{
    //! \todo Here we do the clean up on connection close.
	SConnectInfo* con_info = (SConnectInfo*) *con_cls;

    //printf("request_completed\n-----------------------\n");

    if (NULL != con_info)
        delete con_info;
    con_info = NULL;
    *con_cls = NULL;
}


//////////////////////////////////////////////////////////////////////////////////
/*int GetPostData(SConnectInfo* con_info, const char* upload_data, size_t* upload_data_size)
{
	if ( *upload_data_size != 0 )
	{

	}
}


*/
