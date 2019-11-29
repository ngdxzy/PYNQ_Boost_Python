#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <boost/python.hpp>


using namespace boost::python;
using namespace std;
namespace bp = boost::python;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

typedef struct{
	volatile u32 data[448];
}Vernier_Data;

typedef struct{
	u32 en;
	u32 average;
	u32 phase_counter;
	u8 r_id;
	u8 w_id;
	u8 reserved0;
	u8 reserved1;
	u32 w_addr;
	u32 GTH_DATA;
	u32 reserved3;
	u32 reserved4;
	u32 data[8];
}Vernier_Ctrl;
char buffer[128];
inline void ttyPrint(char* s)
{
	sprintf(buffer,"echo %s > /dev/ttyPS0",s);
	system(buffer);
}

class GTH_System {
private:
public:
	unsigned int mem_fd;
	Vernier_Data* GTH_Data;
	Vernier_Ctrl* GTH_Ctrl;
	int shift_counter;
	GTH_System(u32 ctrl_addr,u32 data_addr)
	{
		mem_fd = open("/dev/mem", O_RDWR);
		GTH_Data = (Vernier_Data*)mmap(0x00,sizeof(Vernier_Data) , PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, data_addr);
		GTH_Ctrl = (Vernier_Ctrl*)mmap(0x00,sizeof(Vernier_Ctrl) , PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, ctrl_addr);
	}

	~GTH_System()
	{
		munmap(GTH_Data,sizeof(Vernier_Data));
		munmap(GTH_Ctrl,sizeof(Vernier_Ctrl));
		close(mem_fd);
	}

	u32 read_bare_data(u8 i)
	{
		if(i < 8)
		{
			return GTH_Ctrl->data[i];
		}
		else
		{
			return -1;
		}
	}

	u32 read_data(u32 i)
	{
		if(i < 448)
		{
			return GTH_Data->data[i];
		}
		else
		{
			return -1;
		}
	}

	void start_capture(const char* name)
	{
		FILE* fp;
		int j;
		fp = fopen(name,"wb");
		if(fp == NULL)
			return ;
		for (j = 0;j < 448;j++)
			fprintf(fp,"%d\n",GTH_Data->data[j]);
		fflush(fp);
		fclose(fp);
		return ;
	}
	
	void send_out_package()
	{
		char buffer[128];
		int j;
		ttyPrint("start:");
		for (j = 0;j < 448;j++)
		{
			sprintf(buffer,"%d",GTH_Data->data[j]);
			ttyPrint(buffer);
		}
	}
	void set_gth_data(u32 d)
	{
		GTH_Ctrl->GTH_DATA = d;
	}
	u32 get_gth_data()
	{
		return GTH_Ctrl->GTH_DATA;
	}
	void enable()
	{
		GTH_Ctrl->en = 1;
	}

	void disable()
	{
		GTH_Ctrl->en = 0;
	}

	void set_average(u32 average)
	{
		GTH_Ctrl->average = average;
	}

	u8 get_wid()
	{
		return GTH_Ctrl->w_id;
	}

	u8 get_rid()
	{
		return GTH_Ctrl->r_id;
	}

	u32 get_phasecounter()
	{
		return GTH_Ctrl->phase_counter;
	}
	
	void capture_to_numpy(boost::python::object obj)
	{
		PyObject* pobj = obj.ptr();
		Py_buffer pybuf;
		if(PyObject_GetBuffer(pobj, &pybuf, PyBUF_SIMPLE)!=-1)
		{
		    void *buf = pybuf.buf;
		    u32 *p = (u32*)buf;
		    u32 *q = (u32*)GTH_Data->data;
		    for (int j = 0;j < 448;j++)
				*p++ = *q++;
		    PyBuffer_Release(&pybuf);
		}
	}
	u32 get_average(u32 average)
	{
		return GTH_Ctrl->average;
	}
};

BOOST_PYTHON_MODULE(GTH_Vernier)
{
	class_<GTH_System>("GTH_System", init<u32,u32>())
		.def(init<u32,u32>())
		.def("start_capture",&GTH_System::start_capture)
		.def("read_data",&GTH_System::read_data)
		.def("read_bare_data",&GTH_System::read_bare_data)
		.def("enable",&GTH_System::enable)
		.def("disable",&GTH_System::disable)
		.def("set_average",&GTH_System::set_average)
		.def("set_gth_data",&GTH_System::set_gth_data)
		.def("get_gth_data",&GTH_System::get_gth_data)
		.def("send_out_package",&GTH_System::send_out_package)
		.def("capture_to_numpy",&GTH_System::capture_to_numpy)
		.def("get_wid",&GTH_System::get_wid)
		.def("get_rid",&GTH_System::get_rid)
		.def_readonly("mem_fd",&GTH_System::mem_fd)
		;
}
