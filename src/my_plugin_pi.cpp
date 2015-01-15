
#include "wx/wxprec.h"

#ifndef  WX_PRECOMP
  #include "wx/wx.h"
  #ifdef ocpnUSE_GL
      #include <wx/glcanvas.h>
  #endif
#endif //precompiled headers

#include <wx/fileconf.h>
#include <wx/stdpaths.h>

////////////////////////////////////////////////
#include <wx/sstream.h>
#include <wx/protocol/http.h>
#include <wx/jsonreader.h>
//////////////////////////////////////////

#include "my_plugin_pi.h"
#include <map>
#include <string>
#include <vector>

std::map<std::string,PlugIn_Waypoint*> myMap;
std::vector<MyMarkerType> markersList;

//---------------------------------------------------------------------------------------------------------
//
//    GoodAnchorage PlugIn Implementation
//
//---------------------------------------------------------------------------------------------------------



// the class factories, used to create and destroy instances of the PlugIn

extern "C" DECL_EXP opencpn_plugin* create_pi(void *ppimgr) {
    return new my_plugin_pi(ppimgr);
}


extern "C" DECL_EXP void destroy_pi(opencpn_plugin* p) {
    delete p;
}



#include "icons.h"


//---------------------------------------------------------------------------------------------------------
//
//          PlugIn initialization and de-init
//
//---------------------------------------------------------------------------------------------------------

my_plugin_pi::my_plugin_pi(void *ppimgr)
    :opencpn_plugin_112(ppimgr)
{
      // Create the PlugIn icons
      initialize_images();
	  
	
	  isPlugInActive = false;
     
}

my_plugin_pi::~my_plugin_pi(void)
{
	  delete _img_ga_anchor_cyan_25;
      delete _img_ga_anchor_cyan_30;
      delete _img_ga_toolbar_off;
}

int my_plugin_pi::Init(void)
{
	isPlugInActive =false;
	m_ActiveMarker = NULL;
	m_ActiveMyMarker = NULL;
	
      AddLocaleCatalog( _T("opencpn-my_plugin_pi") );

      

      //    Get a pointer to the opencpn configuration object
      m_pconfig = GetOCPNConfigObject();

    

      // Get a pointer to the opencpn display canvas, to use as a parent for the GoodAnchorage dialog
      m_parent_window = GetOCPNCanvasWindow();
	  
      AddCustomWaypointIcon(_img_ga_anchor_cyan_25, _T("_img_ga_anchor_cyan_25"), _T("Anchorage"));	 


      //    This PlugIn needs a toolbar icon, so request its insertion if enabled locally
      m_leftclick_tool_id = InsertPlugInTool(_T(""), _img_ga_toolbar_off, _img_ga_toolbar_off, wxITEM_CHECK,
                                                 _("GoodAnchorage"), _T(""), NULL,
                                                 MY_PLUGIN_TOOL_POSITION, 0, this);
			
		
      return (WANTS_OVERLAY_CALLBACK |
              WANTS_OPENGL_OVERLAY_CALLBACK |
              WANTS_CURSOR_LATLON       |
              WANTS_TOOLBAR_CALLBACK    |
              INSTALLS_TOOLBAR_TOOL     |
              WANTS_CONFIG              |
              WANTS_PREFERENCES         |
              WANTS_PLUGIN_MESSAGING    |
              WANTS_MOUSE_EVENTS
            );
}

bool my_plugin_pi::DeInit(void)
{
    cleanMarkerList();
	
    return true;
}

int my_plugin_pi::GetAPIVersionMajor()
{
      return MY_API_VERSION_MAJOR;
}

int my_plugin_pi::GetAPIVersionMinor()
{
      return MY_API_VERSION_MINOR;
}

int my_plugin_pi::GetPlugInVersionMajor()
{
      return PLUGIN_VERSION_MAJOR;
}

int my_plugin_pi::GetPlugInVersionMinor()
{
      return PLUGIN_VERSION_MINOR;
}

wxBitmap *my_plugin_pi::GetPlugInBitmap()
{
      return _img_ga_toolbar_off;
}

wxString my_plugin_pi::GetCommonName()
{
      return _T("GoodAnchorage");
}


wxString my_plugin_pi::GetShortDescription()
{
      return _("GoodAnchorage PlugIn for OpenCPN");
}


wxString my_plugin_pi::GetLongDescription()
{
      return _("GoodAnchorage PlugIn for OpenCPN\n\
Provides access to GoodAnchorage.com data." );
}


void my_plugin_pi::SetDefaults(void)
{
}


int my_plugin_pi::GetToolbarToolCount(void)
{
      return 1;
}

bool my_plugin_pi::MouseEventHook( wxMouseEvent &event )
{
   if(!isPlugInActive)
		return false;
		
	
	if( event.LeftDClick() && m_ActiveMarker == NULL )
	{
		double plat,plon;
		
        GetCanvasLLPix( &m_vp, event.GetPosition(), &plat, &plon );
		//wxMessageBox( wxString::Format(wxT("%f"),plat) + _T(" ") + wxString::Format(wxT("%f"),plon));
		sendRequest(plat,plon);
		
		return true;
	}
	else if( m_ActiveMarker != NULL )
	{
		
		
		if( event.LeftDClick() )
		{
		
			if(m_ActiveMyMarker !=  NULL)
				sendRequestPlus(m_ActiveMyMarker->serverId);
			
			return true;
		}
		 
		if( event.RightDown() ) 
			return true;
			
		if( event.LeftDown() )
			return true;
		
		
		return false;
	}
	else
		return false;
    
}

void my_plugin_pi::ShowPreferencesDialog( wxWindow* parent )
{
    
}

void my_plugin_pi::OnToolbarToolCallback(int id)
{
	if(isPlugInActive)
	{
		 cleanMarkerList();
		isPlugInActive = false;
		return ;
	}
	else
	{
		isPlugInActive = true;
	}

	/*
	PlugIn_Waypoint *pwaypoint = new PlugIn_Waypoint( 0.0, 0.0,
                    _T("icon_ident"), _T("wp_name"),
                     _T("GUID") );				 
	AddSingleWaypoint(  pwaypoint,  true);

	
	PlugIn_Waypoint *pwaypoint1 = new PlugIn_Waypoint( 0.1, 0.1,
                    _T("icon_ident"), _T("wp_name"),
                     _T("GUID1") );				 
	AddSingleWaypoint(  pwaypoint1,  true);

	
	PlugIn_Waypoint *pwaypoint2 = new PlugIn_Waypoint( 0.5, 0.5,
                    _T("icon_ident"), _T("wp_name"),
                     _T("GUID2") );					 
	AddSingleWaypoint(  pwaypoint2,  true);
	
	
	myMap["GUID"] = pwaypoint;
	myMap["GUID1"] = pwaypoint1;
	myMap["GUID2"] = pwaypoint2;
	
	*/
	
    RequestRefresh(m_parent_window); // refresh mainn window
	
}



bool my_plugin_pi::RenderOverlay(wxDC &dc, PlugIn_ViewPort *vp)
{
    m_vp = *vp;
    return true;
}

bool my_plugin_pi::RenderGLOverlay(wxGLContext *pcontext, PlugIn_ViewPort *vp)
{
    m_vp = *vp;
    return true;
}

void my_plugin_pi::SetCursorLatLon(double lat, double lon)
{


	if(!isPlugInActive)
		return ;
		
	
		
	wxPoint cur;
    GetCanvasPixLL( &m_vp, &cur, lat, lon );
	
	
	bool changeMarkerFlag = false;
	for (std::vector<MyMarkerType>::iterator it = markersList.begin() ; it != markersList.end(); ++it)
	//for (std::map<std::string,PlugIn_Waypoint*>::iterator it=myMap.begin(); it!=myMap.end(); ++it)
	{
		
		//PlugIn_Waypoint *marker = it->second;
		PlugIn_Waypoint *marker = it->pluginWaitPoint;
		
		 if(  PointInLLBox( &m_vp,  marker->m_lon, marker->m_lat ) ) // !!!!!!!!!!!!!!! x == lon  and y = lat !!!!!!!!
		 {
		 			
			//wxMessageBox(wxString::Format(wxT("%f"),marker->m_lat) + _T(" ") +wxString::Format(wxT("%f"),marker->m_lon));
			wxPoint pl;
            GetCanvasPixLL( &m_vp, &pl, marker->m_lat, marker->m_lon );
            if (pl.x > cur.x - 10 && pl.x < cur.x + 10 && pl.y > cur.y - 10 && pl.y < cur.y + 10)
            {
			
				if(m_ActiveMyMarker != NULL)
				{
					m_ActiveMyMarker->pluginWaitPoint->m_MarkName = _T("");
					UpdateSingleWaypoint(m_ActiveMyMarker->pluginWaitPoint);
				}
				
                m_ActiveMarker = marker;
				m_ActiveMyMarker = &(*it);
                changeMarkerFlag = true;
				
				m_ActiveMyMarker->pluginWaitPoint->m_MarkName = m_ActiveMyMarker->getMarkerTitle();
				UpdateSingleWaypoint(m_ActiveMyMarker->pluginWaitPoint);
				
                break;
            }
		 }
	}
	
	if(	!changeMarkerFlag	)
	{
		if(m_ActiveMyMarker != NULL)
		{
			m_ActiveMyMarker->pluginWaitPoint->m_MarkName = _T("");
			UpdateSingleWaypoint(m_ActiveMyMarker->pluginWaitPoint);
		}
	
		m_ActiveMarker = NULL;
		m_ActiveMyMarker = NULL;
	}
  
}

bool my_plugin_pi::PointInLLBox( PlugIn_ViewPort *vp, double x, double y )
{
    double Marge = 0.;
    double m_minx = vp->lon_min;
    double m_maxx = vp->lon_max;
    double m_miny = vp->lat_min;
    double m_maxy = vp->lat_max;

    //    Box is centered in East lon, crossing IDL
    if(m_maxx > 180.)
    {
        if( x < m_maxx - 360.)
            x +=  360.;

        if (  x >= (m_minx - Marge) && x <= (m_maxx + Marge) &&
            y >= (m_miny - Marge) && y <= (m_maxy + Marge) )
            return TRUE;
        return FALSE;
    }

    //    Box is centered in Wlon, crossing IDL
    else if(m_minx < -180.)
    {
        if(x > m_minx + 360.)
            x -= 360.;

        if (  x >= (m_minx - Marge) && x <= (m_maxx + Marge) &&
            y >= (m_miny - Marge) && y <= (m_maxy + Marge) )
            return TRUE;
        return FALSE;
    }

    else
    {
        if (  x >= (m_minx - Marge) && x <= (m_maxx + Marge) &&
            y >= (m_miny - Marge) && y <= (m_maxy + Marge) )
            return TRUE;
        return FALSE;
    }
	/*
	else
    {
        if (  x >= (m_minx - Marge) && x <= (m_maxx + Marge) &&
            y >= (m_miny - Marge) && y <= (m_maxy + Marge) )
            return TRUE;
        return FALSE;
    }
	*/
}

void my_plugin_pi::OnContextMenuItemCallback(int id)
{
   
}


void my_plugin_pi::SetPluginMessage(wxString &message_id, wxString &message_body)
{
    
}


void my_plugin_pi::SendTimelineMessage(wxDateTime time)
{
   
}


void my_plugin_pi::initLoginDialog(wxWindow* parent)
{

	
}


void my_plugin_pi::sendRequest(double lat,double lon){

	double m_minx = m_vp.lon_min;
    double m_maxx = m_vp.lon_max;
    double m_miny = m_vp.lat_min;
    double m_maxy = m_vp.lat_max;
	
	/*wxMessageBox(
	 wxString::Format(wxT("m_minx = %f "),m_minx)+
	 wxString::Format(wxT("m_maxx = %f "),m_maxx)+
	 wxString::Format(wxT("m_miny = %f "),m_miny)+
	 wxString::Format(wxT("m_maxy = %f "),m_maxy)
	 );*/
	 
	
	cleanMarkerList();

	wxHTTP get;
	get.SetHeader(_T("Content-type"), _T("text/html; charset=utf-8"));
	get.SetTimeout(10); // 10 seconds of timeout instead of 10 minutes ...
	 
	// this will wait until the user connects to the internet. It is important in case of dialup (or ADSL) connections
	while (!get.Connect(_T("goodanchorage.com")))  // only the server, no pages here yet ...
		wxSleep(5);
	 
	wxApp::IsMainLoopRunning(); // should return true
	 
	 //goodanchorage.com/api/v1/anchor_list/
	// use _T("/") for index.html, index.php, default.asp, etc.
	wxInputStream *httpStream = get.GetInputStream( _T("/api/v1/anchor_list/") + wxString::Format(wxT("%f"),lat) + _T("/") + wxString::Format(wxT("%f.json"),lon) );
	 
	// wxLogVerbose( wxString(_T(" GetInputStream: ")) << get.GetResponse() << _T("-") << ((resStream)? _T("OK ") : _T("FAILURE ")) << get.GetError() );
	 
	if (get.GetError() == wxPROTO_NOERR)
	{
		wxString res;
		wxStringOutputStream out_stream(&res);
		httpStream->Read(out_stream);
	 
	 
		// construct the JSON root object
            wxJSONValue  root;
        // construct a JSON parser
            wxJSONReader reader;

        // now read the JSON text and store it in the 'root' structure
        // check for errors before retreiving values...
            int numErrors = reader.Parse( res, &root );
            if ( numErrors > 0 || !root.IsArray() )  {
				
				wxMessageBox(_T("!root.IsArray() ") + res);
				
                return;
            }
			
			
			for ( int i = 0; i < root.Size(); i++ )  {
			
				bool is_deep =  root[i][_T("is_deep")].AsBool();
				int id = root[i][_T("id")].AsInt();
				wxString title = root[i][_T("title")].AsString();
				
				 wxJSONValue lat_lon = root[i][_T("lat_lon")];
				 
				 if ( !lat_lon.IsArray() ) {
						
						wxMessageBox(_T("!lat_lon.IsArray()) ") + res);
						
						return ;
				}

				double lat_i = lat_lon[0].AsDouble();
				double lon_i = lat_lon[1].AsDouble();

				MyMarkerType newMarker;
				
				newMarker.serverDeep = is_deep;
				newMarker.serverId = id;
				newMarker.serverTitle = title;
				newMarker.serverLat = lat_i;
				newMarker.serverLon = lon_i;

				
				PlugIn_Waypoint *bufWayPoint = new PlugIn_Waypoint( lat_i, lon_i,
                    _T("icon_ident"), _T("") ,
                      GetNewGUID()  );
					 
				newMarker.pluginWaitPoint = bufWayPoint;
				
				markersList.push_back(newMarker);
			 
			}
			
			
			showMarkerList();
			wxMessageBox(_T("OK"));
			
		
		
	}
	else
	{
		wxMessageBox(wxString(_T("Unable to connect! Error code: ")) <<  get.GetError());
	}
	 
	wxDELETE(httpStream);
	get.Close();
}


void my_plugin_pi::sendRequestPlus(int id){

	

	wxHTTP get;
	get.SetHeader(_T("Content-type"), _T("text/html; charset=utf-8"));
	get.SetTimeout(10); // 10 seconds of timeout instead of 10 minutes ...
	 
	// this will wait until the user connects to the internet. It is important in case of dialup (or ADSL) connections
	while (!get.Connect(_T("goodanchorage.com")))  // only the server, no pages here yet ...
		wxSleep(5);
	 
	wxApp::IsMainLoopRunning(); // should return true
	 
	 //goodanchorage.com/api/v1/anchor_list/
	// use _T("/") for index.html, index.php, default.asp, etc.
	wxInputStream *httpStream = get.GetInputStream( _T("/api/v1/anchor_point/") + wxString::Format(wxT("%d.json"),id)  );
	 
	// wxLogVerbose( wxString(_T(" GetInputStream: ")) << get.GetResponse() << _T("-") << ((resStream)? _T("OK ") : _T("FAILURE ")) << get.GetError() );
	 
	if (get.GetError() == wxPROTO_NOERR)
	{
		wxString forPrint;
		wxString res;
		wxStringOutputStream out_stream(&res);
		httpStream->Read(out_stream);
	 
	 
		// construct the JSON root object
            wxJSONValue  root;
        // construct a JSON parser
            wxJSONReader reader;

        // now read the JSON text and store it in the 'root' structure
        // check for errors before retreiving values...
            int numErrors = reader.Parse( res, &root );
            if ( numErrors > 0  )  {
				
				wxMessageBox(_T("!root.IsArray() ") + res);
				
                return;
            }
			
			if( root[_T("id")].IsValid() && !root[_T("id")].IsNull() )
				int id = root[_T("id")].AsInt();
				
			if( root[_T("title")].IsValid() && !root[_T("title")].IsNull() )
			{
				wxString title = root[_T("title")].AsString(); //Anchorage Name
				forPrint += _T("Anchorage Name: ") + title + _T("\n");
			}

			if( root[_T("title_alt")].IsValid() && !root[_T("title_alt")].IsNull() )
			{
				wxString title_alt = root[_T("title_alt")].AsString() ; //Alternative Name
				forPrint += _T("Alternative Name: ") + title_alt + _T("\n");
			}

		 
			if( root[_T("lat_lon")].IsValid() && !root[_T("lat_lon")].IsNull() )
			{
				wxJSONValue lat_lon = root[_T("lat_lon")];//Anchorage Position Single lat/lon pair
					 
				if ( !lat_lon.IsArray() ) 
				{					
					wxMessageBox(_T("!lat_lon.IsArray()) ") + res);						
					return ;
				}

				double lat_i = lat_lon[0].AsDouble();
				double lon_i = lat_lon[1].AsDouble();
				
				forPrint += _T("Anchorage Position: ") 
					+ wxString::Format(wxT(" %.3f/"),lat_i)
					+ wxString::Format(wxT(" %.3f"),lon_i)
					+ _T("\n");
			}
			
			if( root[_T("wp_lat_lon")].IsValid() && !root[_T("wp_lat_lon")].IsNull() )
			{
				wxJSONValue wp_lat_lon  = root[_T("wp_lat_lon")]; //Safe Waypoint(s) Multiple lat/lon pairs
				
				if ( !wp_lat_lon.IsArray() ) 
				{					
					wxMessageBox(_T("!wp_lat_lon.IsArray()) ") + res);						
					return ;
				}
				
				forPrint += _T("Safe Waypoint(s): ") ;
				
				for(int i=0; i < wp_lat_lon.Size();i++)
				{
					double lat_wp_i = wp_lat_lon[i][0].AsDouble();
					double lon_wp_i = wp_lat_lon[i][0].AsDouble();
					
					
					forPrint += wxString::Format(wxT(" %.3f"),lat_wp_i)
						+ wxString::Format(wxT(" %.3f"),lon_wp_i)
						+ _T(" ; ");
				}
				
				forPrint += _T("\n"); 
			}

			if( root[_T("depth_m")].IsValid() && !root[_T("depth_m")].IsNull() )
			{
				double depth_m = root[_T("depth_m")].AsDouble(); //Depth (m)
				
				forPrint += _T("Depth (m): ") 
					+ wxString::Format(wxT(" %.1f"),depth_m)
					+ _T("\n");
			}

			if( root[_T("depth_ft")].IsValid() && !root[_T("depth_ft")].IsNull() )
			{
				double depth_ft = root[_T("depth_ft")].AsDouble(); //Depth (ft)
				
				forPrint += _T("Depth (ft): ") 
					+ wxString::Format(wxT(" %.1f"),depth_ft)
					+ _T("\n");
			}

				
			if( root[_T("is_deep")].IsValid() && !root[_T("is_deep")].IsNull() )
				bool is_deep = root[_T("is_deep")].AsBool() ; //Don't show

			if( root[_T("bottom")].IsValid() && !root[_T("bottom")].IsNull() )
			{			
				wxJSONValue bottom  = root[_T("bottom")]; //Bottom Composition
				
				forPrint += _T("Bottom Composition: ") ;
				for(int i=0; i < bottom.Size();i++)
				{
					forPrint += bottom[i].AsString() + _T(" ");
				}
				forPrint += _T("\n"); 
			}
			
			if( root[_T("prot_wind")].IsValid() && !root[_T("prot_wind")].IsNull() )
			{
				wxJSONValue prot_wind  = root[_T("prot_wind")]; //Protection from Wind
				
				forPrint += _T("Protection from Wind: ") ;
				for(int i=0; i < prot_wind.Size();i++)
				{
					forPrint += prot_wind[i].AsString() + _T(" ");
				}
				forPrint += _T("\n"); 
			}

			if( root[_T("prot_swell")].IsValid() && !root[_T("prot_swell")].IsNull() )
			{
				wxJSONValue prot_swell  = root[_T("prot_swell")]; //Protection from Swell
				
				forPrint += _T("Protection from Swell: ") ;
				for(int i=0; i < prot_swell.Size();i++)
				{
					forPrint += prot_swell[i].AsString() + _T(" ");
				}
				forPrint += _T("\n"); 
			}

			if( root[_T("navigation")].IsValid() && !root[_T("navigation")].IsNull() )
			{
				wxJSONValue navigation  = root[_T("navigation")]; //Navigation Assets
				
				forPrint += _T("Navigation Assets: ") ;
				for(int i=0; i < navigation.Size();i++)
				{
					forPrint += navigation[i].AsString() + _T(" ");
				}
				forPrint += _T("\n"); 
			}

			if( root[_T("shore_access")].IsValid() && !root[_T("shore_access")].IsNull() )
			{
				wxJSONValue shore_access  = root[_T("shore_access")]; //Shore Access
				
				forPrint += _T("Shore Access: ") ;
				for(int i=0; i < shore_access.Size();i++)
				{
					forPrint += shore_access[i].AsString() + _T(" ");
				}
				forPrint += _T("\n"); 
			}

			if( root[_T("closeby")].IsValid() && !root[_T("closeby")].IsNull() )
			{
				wxString closeby  = root[_T("closeby")].AsString(); //Close-by
				
				forPrint += _T("Close-by: ") 
					+ closeby
					+ _T("\n");
			}

			if( root[_T("nearby_town")].IsValid() && !root[_T("nearby_town")].IsNull() )
			{
				wxString nearby_town = root[_T("nearby_town")].AsString(); // Nearest Town(s)
				
				forPrint += _T("Nearest Town(s): ") 
					+ nearby_town
					+ _T("\n");
			}

			if( root[_T("dangers")].IsValid() && !root[_T("dangers")].IsNull() )
			{
				wxJSONValue dangers  = root[_T("dangers")]; //Dangers
				
				forPrint += _T("Dangers: ") ;
				for(int i=0; i < dangers.Size();i++)
				{
					forPrint += dangers[i].AsString() + _T(" ");
				}
				forPrint += _T("\n"); 
			}

			if( root[_T("country")].IsValid() && !root[_T("country")].IsNull() )
			{
				wxString country  = root[_T("country")].AsString(); //Country
				
				forPrint += _T("Country: ") 
					+ country
					+ _T("\n");
			}

			if( root[_T("date_anchored")].IsValid() && !root[_T("date_anchored")].IsNull() )
			{
				wxString date_anchored  = root[_T("date_anchored")].AsString(); //Date Anchored
				
				forPrint += _T("Date Anchored: ") 
					+ date_anchored
					+ _T("\n");
			}

			if( root[_T("comments")].IsValid() && !root[_T("comments")].IsNull() )
			{
				wxString comments  = root[_T("comments")].AsString(); //Comments
				
				forPrint += _T("Comments: ") 
					+ comments
					+ _T("\n");
			}
			
			
			
			wxMessageBox(forPrint);
		
	}
	else
	{
		wxMessageBox(_T("Unable to connect!"));
	}
	 
	wxDELETE(httpStream);
	get.Close();
}

void my_plugin_pi::cleanMarkerList(void)
{
	m_ActiveMarker = NULL;
	m_ActiveMyMarker = NULL;

	for(unsigned int i = 0; i < markersList.size(); i++)
	{
		DeleteSingleWaypoint(  markersList[i].pluginWaitPoint->m_GUID );
	}
	
	markersList.clear();
}

void my_plugin_pi::showMarkerList(void)
{

	for(unsigned int i = 0; i < markersList.size(); i++)
	{
		AddSingleWaypoint(  markersList[i].pluginWaitPoint,  true);
	}
}
	
	


MyMarkerType::MyMarkerType(void){}
MyMarkerType::~MyMarkerType(void){}

wxString MyMarkerType::getMarkerTitle(void)
{
	wxString result = this->serverTitle 
	
	+wxString::Format(wxT(" (%.3f "),this->serverLat)
	+wxString::Format(wxT(" , %.3f / "),this->serverLon) ;
	
	if(this->serverDeep)
	{
		result += _T("deep)");
	}
	else
	{
		result += _T("not deep)");
	}
	
	return result;
}