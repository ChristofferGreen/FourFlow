#ifndef ConnectionWindow_h
#define ConnectionWindow_h

#include "ui_ConnectionWindow.h"
#include "qtIncludes.h"
#include "pqIncludes.h"
#include "cppIncludes.h"
#include <QMainWindow>

class FourFlowMainWindow;

class ConnectionWindow : public QMainWindow {
	Q_OBJECT
	typedef QMainWindow Superclass;
	public:
		enum Type {
			ParticleTrace=0,
			PathLines,
			StreamLines,
			VolumeTracking,
			Graph,
			ParticleCollector,
		};
		FourFlowMainWindow *ffWindow;
		QSharedPointer<Ui_ConnectionWindow> internals;

		ConnectionWindow();
		~ConnectionWindow() {}

		void setInputText(const QString &title, const QString &help);
		void setSourceText(const QString &title, const QString &help);
		void setType(Type type);
		void fill();
		void clear();
		virtual void show();
	protected slots:
		void nextClicked();
	private:
		void execute();
		bool attemptToDeduceInputsAutomatically();
		QString sourceTitle;
		QString sourceText;
		QString inputTitle;
		QString inputText;
		Type type;
		std::vector<pqPipelineSource*> inputs;
		std::vector<pqPipelineSource*> sources;
		pqPipelineSource *selectedInput;
		pqPipelineSource *selectedSource;
};

#endif