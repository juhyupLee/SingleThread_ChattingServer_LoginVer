#include "CPUUsage.h"

CPUUsage::CPUUsage(HANDLE process)
{
	//-----------------------------------------------
	// 	   ���μ��� �ڵ� �Է��� ���ٸ� �ڱ��ڽ��� �������
	//-----------------------------------------------

	if (process == INVALID_HANDLE_VALUE)
	{
		m_Process = GetCurrentProcess();
	}
	//-----------------------------------------------
	// 	���μ��� ������ Ȯ���Ѵ�
	// 	���μ��� (exe) ������ ����, CPU ������ �����⸦ �Ͽ� ���� ������� ����
	//-----------------------------------------------

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	m_NuberOfProcessors = sysInfo.dwNumberOfProcessors;


	m_ProcessorTotal = 0;
	m_ProcessorUser = 0;
	m_ProcessorKernel = 0;

	m_ProcessTotal = 0;
	m_ProcessUser = 0;
	m_ProcessKernel = 0;

	m_Processor_LastKernel.QuadPart = 0;
	m_Processor_LastUser.QuadPart = 0;
	m_Processor_LastIdle.QuadPart = 0;

	m_Process_LastUser.QuadPart = 0;
	m_Process_LastKernel.QuadPart = 0;
	m_Process_LastTime.QuadPart = 0;

	UpdateCPUTime();


}

void CPUUsage::UpdateCPUTime()
{
	//-------------------------------------------------------------
	// 	   ���μ��� ������� �����Ѵ�
	// 	   ������ ��� ����ü�� FILETIME ������, ULARGE_INTER��  ������ �����Ƿ� �̸� �����
	// 	   FILETIME ����ü�� 100 ���뼼���� ������ �ð� ������ ǥ���ϴ� ����ü��
	//-------------------------------------------------------------
	
	ULARGE_INTEGER idle;
	ULARGE_INTEGER kernel;
	ULARGE_INTEGER user;

	//-------------------------------------------------------------
	// 	   �ý��� ��� �ð��� ���Ѵ�.
	// 	   ���̵� Ÿ�� / Ŀ�� ��� Ÿ�� (���̵�����) / ���� ���Ÿ��
	//-------------------------------------------------------------

	if (GetSystemTimes((PFILETIME)&idle, (PFILETIME)&kernel, (PFILETIME)&user) == false)
	{
		return;
	}

	//-------------------------------------------------------------
	// 	  Ŀ�� Ÿ�ӿ���  ���̵� Ÿ���� ���Ե�
	//-------------------------------------------------------------		
	ULONGLONG kernelDiff = kernel.QuadPart - m_Processor_LastKernel.QuadPart;
	ULONGLONG userDiff = user.QuadPart - m_Processor_LastUser.QuadPart;
	ULONGLONG idleDiff = idle.QuadPart - m_Processor_LastIdle.QuadPart;


	ULONGLONG total = kernelDiff + userDiff;
	ULONGLONG timeDiff;

	if (total == 0)
	{
		m_ProcessorUser = 0.0f;
		m_ProcessorKernel = 0.0f;
		m_ProcessorTotal = 0.0f;
	}
	else
	{
		// Ŀ��Ÿ�ӿ� ���̵� Ÿ���� �����Ƿ� ���� ���
		m_ProcessorTotal = (float)((double)(total - idleDiff) / total * 100.0f);
		m_ProcessorUser = (float)((double)userDiff / total * 100.0f);
		m_ProcessorKernel = (float)((double)(kernelDiff - idleDiff) / total * 100.0f);
	}

	m_Processor_LastKernel = kernel;
	m_Processor_LastUser = user;
	m_Processor_LastIdle = idle;

	//-------------------------------------------------------------
	// 	  ������ ���μ��� ������ �����Ѵ�
	//-------------------------------------------------------------	
	ULARGE_INTEGER none;
	ULARGE_INTEGER nowTime;

	//-------------------------------------------------------------
	// 	  ������ 100 ���� ������ ���� �ð��� ���Ѵ� UTC �ð�
	// 	   ���μ��� ����� �Ǵ��� ����
	// 
	// 	   a = ���ð����� �ý��� �ð��� ����(�׳� ������ ������ �ð�)
	// 	   b = ���μ����� CPU��� �ð��� ����
	// 
	// 	   a: 100  = b: �����  
	//-------------------------------------------------------------	


	//-------------------------------------------------------------
	// 	 ���� �ð��� �������� 100���� ������ �ð��� ����
	//-------------------------------------------------------------
	GetSystemTimeAsFileTime((LPFILETIME)&nowTime);


	//-------------------------------------------------------------
	// 	 �ش� ���μ����� ����� �ð��� ����
	// 	 �ι�°, ����°�� ����, ���� �ð����� �̻��
	//-------------------------------------------------------------
	GetProcessTimes(m_Process,(LPFILETIME)&none, (LPFILETIME)&none, (LPFILETIME)&kernel, (LPFILETIME)&user);


	//-------------------------------------------------------------
	//	������ ����� ���μ��� �ð����� ���� ���ؼ� ������ ���� �ð��� �������� Ȯ��
	// 	�׸��� ���� ������ �ð����� ������ ������ ����
	//-------------------------------------------------------------

	timeDiff = nowTime.QuadPart - m_Process_LastTime.QuadPart;
	userDiff = user.QuadPart - m_Process_LastUser.QuadPart;
	kernelDiff = kernel.QuadPart - m_Process_LastKernel.QuadPart;

	total = kernelDiff + userDiff;

	m_ProcessTotal = (float)(total / (double)m_NuberOfProcessors / (double)timeDiff * 100.0f);
	m_ProcessKernel = (float)(kernelDiff / (double)m_NuberOfProcessors / (double)timeDiff * 100.0f);
	m_ProcessUser = (float)(userDiff / (double)m_NuberOfProcessors / (double)timeDiff * 100.0f);

	m_Process_LastTime = nowTime;
	m_Process_LastKernel = kernel;
	m_Process_LastUser = user;

}

float CPUUsage::ProcessorTotal()
{
	return m_ProcessorTotal;
}

float CPUUsage::ProcessorUser()
{
	return m_ProcessorUser;
}

float CPUUsage::ProcessorKernel()
{
	return m_ProcessorKernel;
}

float CPUUsage::ProcessTotal()
{
	return m_ProcessTotal;
}

float CPUUsage::ProcessUser()
{
	return m_ProcessUser;
}

float CPUUsage::ProcessKernel()
{
	return m_ProcessKernel;
}
