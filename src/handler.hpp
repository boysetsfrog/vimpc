#ifndef __UI__HANDLER
#define __UI__HANDLER

namespace Ui
{
	class Handler
	{
	public:
		virtual ~Handler() { }

	public:
		virtual void InitialiseMode()  = 0;
		virtual void FinaliseMode()    = 0;
		virtual bool Handle(int input) = 0;
	};
}

#endif
