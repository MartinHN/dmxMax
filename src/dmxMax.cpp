//
//  dmxMax.cpp
//  dmxMax
//
//  Created by Martin Hermant on 03/01/2016.
//
//
#include "ext.h"
#include "pro_driver.h"
#include "ext_obex.h"
#include "ext_strings.h"
#include "ext_common.h"
#include "ext_systhread.h"

//#include "ext_obex.h"
//#include "ext_strings.h"
//#include "ext_common.h"
//#include "ext_systhread.h"

// a macro to mark exported symbols in the code without requiring an external file to define them
#ifdef WIN_VERSION
// note that this is the required syntax on windows regardless of whether the compiler is msvc or gcc
#define T_EXPORT __declspec(dllexport)
#else // MAC_VERSION
// the mac uses the standard gcc syntax, you should also set the -fvisibility=hidden flag to hide the non-marked symbols
#define T_EXPORT __attribute__((visibility("default")))
#endif


// max object instance data
typedef struct _dmxMax {
    t_object			c_box;
    
    DMXUSBPROParamsType PRO_Params;
    FT_HANDLE device_handle = NULL ;
    unsigned char myDmx[DMX_DATA_LENGTH];
    unsigned char myDmxPre[DMX_DATA_LENGTH];
    float grandMaster;
    void				*c_outlet;
    t_systhread_mutex	c_mutex;
} t_dmxMax;




// prototypes
void*	dmxMax_new(t_symbol *s, long argc, t_atom *argv);
void	dmxMax_free(t_dmxMax* x);

//void dmxMax_int(t_dmxMax * x,long i);
void dmxMax_list(t_dmxMax *x, t_symbol *msg, long argc, t_atom *argv);
void dmxMax_circ(t_dmxMax *x, t_symbol *msg, long argc, t_atom *argv);
void dmxMax_open(t_dmxMax *x,long device_num);
void dmxMax_sendValues(t_dmxMax *x);
void dmxMax_bang(t_dmxMax *x);
void dmxMax_grandMaster(t_dmxMax *x,t_atom_float gm );
void dmxMax_listDevices(t_dmxMax * x);
// globals
static t_class	*s_dmxMax_class = NULL;


int T_EXPORT main(void)
{
    t_class	*c = class_new("dmxMax",
                           (method)dmxMax_new,
                           (method)dmxMax_free,
                           sizeof(t_dmxMax),
                           (method)NULL,
                           A_GIMME,
                           0);
    
    class_addmethod(c, (method)dmxMax_bang,	"bang",			0);
    class_addmethod(c, (method)dmxMax_open,	"open",		A_LONG,0);
    class_addmethod(c, (method)dmxMax_list,	"list",			A_GIMME,0);
    class_addmethod(c, (method)dmxMax_circ,	"circ",			A_GIMME,0);
    class_addmethod(c, (method)dmxMax_grandMaster,	"grandMaster",		A_FLOAT,0);
    class_addmethod(c,(method)dmxMax_listDevices, "listDevices", 0);
    
    class_register(CLASS_BOX, c);
    s_dmxMax_class = c;
    
    return 0;
}


void dmxMax_listDevices(t_dmxMax * x){
    int numDevice  = FTDI_ListDevices();
    post("found %i devices" , numDevice);
}

void dmxMax_bang(t_dmxMax *x){
    dmxMax_sendValues(x);
}


bool dmxMax_isConnected(t_dmxMax *x){
    return x->device_handle!=NULL;
}
void dmxMax_sendValues(t_dmxMax * x){
    if(dmxMax_isConnected(x)){
        for(int i = 0; i < DMX_DATA_LENGTH ; i ++){
            x->myDmx[i] = x->myDmxPre[i]*x->grandMaster;
        }
        if( FTDI_SendData(x->device_handle,SET_DMX_TX_MODE, x->myDmx, DMX_DATA_LENGTH)<0){
            {
                post("dmxMax : FAILED: Sending DMX to PRO \n");
                FTDI_ClosePort(x->device_handle);
                x->device_handle = NULL;
                
            };
        }
    }
    else{
        post("dmxMax : can't send Values,no devices found");
    }
}
void dmxMax_open(t_dmxMax *x,long device_num){
    if(!FTDI_OpenDevice(&x->device_handle,&x->PRO_Params,(int)device_num)){
        post("dmxMax : failed to open device number ", device_num);
        x->device_handle = NULL;
    }
    
    
}

void dmxMax_list(t_dmxMax *x, t_symbol *msg, long argc, t_atom *argv){
    
    if(argc>=DMX_DATA_LENGTH){
        post("dmxMax : too many raw values");
    }
    else {
        for(int i = 0; i < argc ; i ++){
            x->myDmxPre[i+1] = argv[i].a_w.w_float;
        }
        dmxMax_sendValues(x);
    }
}

void dmxMax_circ(t_dmxMax *x, t_symbol *msg, long argc, t_atom *argv){
    
    if(argc!=2){
        post("dmxMax : wrong formating circuit values (2 expected : circuit , value)");
    }
    else{
        int circuit = argv[0].a_w.w_float;
        if(circuit>=0 && circuit<DMX_DATA_LENGTH-1){
            x->myDmxPre[circuit+1] = argv[1].a_w.w_float;
            dmxMax_sendValues(x);
        }
        else{
            post("dmxMax : circuit not in [0..512]");
        }
    }
}

void dmxMax_grandMaster(t_dmxMax *x,t_atom_float gm ){
    x->grandMaster = gm;
    dmxMax_sendValues(x);
}



/************************************************************************************/
// Object Creation Method

void *dmxMax_new(t_symbol *s, long argc, t_atom *argv)
{
    t_dmxMax	*x;
    
    x = (t_dmxMax*)object_alloc(s_dmxMax_class);
    if (x) {
        systhread_mutex_new(&x->c_mutex, 0);
        x->c_outlet = outlet_new(x, NULL);
        //        x->c_vector = new numberVector;
        //        x->c_vector->reserve(10);
        //        dmxMax_list(x, gensym("list"), argc, argv);
    }
    return(x);
}


void dmxMax_free(t_dmxMax *x)
{
    systhread_mutex_free(x->c_mutex);
    
}

