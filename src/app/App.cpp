/*
 * App.cpp
 * Copyright (C) 2017-2018  Belledonne Communications, Grenoble, France
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *  Created on: February 2, 2017
 *      Author: Ronan Abhamon
 */

#include <QCommandLineParser>
#include <QDir>
#include <QFileSelector>
#include <QLibraryInfo>
#include <QMenu>
#include <QQmlFileSelector>
#include <QSystemTrayIcon>
#include <QTimer>

#include "config.h"

#include "../components/Components.hpp"
#include "../utils/LinphoneUtils.hpp"
#include "../utils/Utils.hpp"

#include "cli/Cli.hpp"
#include "logger/Logger.hpp"
#include "paths/Paths.hpp"
#include "providers/AvatarProvider.hpp"
#include "providers/ImageProvider.hpp"
#include "providers/ThumbnailProvider.hpp"
#include "translator/DefaultTranslator.hpp"

#include "App.hpp"

using namespace std;

// =============================================================================

namespace {
  constexpr char cDefaultLocale[] = "en";

  constexpr char cLanguagePath[] = ":/languages/";

  // The main windows of Linphone desktop.
  constexpr char cQmlViewMainWindow[] = "qrc:/ui/views/App/Main/MainWindow.qml";
  constexpr char cQmlViewCallsWindow[] = "qrc:/ui/views/App/Calls/CallsWindow.qml";
  constexpr char cQmlViewSettingsWindow[] = "qrc:/ui/views/App/Settings/SettingsWindow.qml";

  constexpr char cQmlViewSplashScreen[] = "qrc:/ui/views/App/SplashScreen/SplashScreen.qml";

  constexpr int cVersionUpdateCheckInterval = 86400000; // 24 hours in milliseconds.
}

static inline bool installLocale (App &app, QTranslator &translator, const QLocale &locale) {
  return translator.load(locale, cLanguagePath) && app.installTranslator(&translator);
}

static inline shared_ptr<linphone::Config> getConfigIfExists (const QCommandLineParser &parser) {
  string configPath = Paths::getConfigFilePath(parser.value("config"), false);
  if (Paths::filePathExists(configPath))
    return linphone::Config::newWithFactory(configPath, "");

  return nullptr;
}

// -----------------------------------------------------------------------------

App::App (int &argc, char *argv[]) : SingleApplication(argc, argv, true, Mode::User | Mode::ExcludeAppPath | Mode::ExcludeAppVersion) {
  setWindowIcon(QIcon(WINDOW_ICON_PATH));

  createParser();
  mParser->process(*this);

  // Initialize logger.
  shared_ptr<linphone::Config> config = ::getConfigIfExists(*mParser);
  Logger::init(config);
  if (mParser->isSet("verbose"))
    Logger::getInstance()->setVerbose(true);

  // List available locales.
  for (const auto &locale : QDir(cLanguagePath).entryList())
    mAvailableLocales << QLocale(locale);

  // Init locale.
  mTranslator = new DefaultTranslator(this);
  initLocale(config);

  if (mParser->isSet("help")) {
    createParser();
    mParser->showHelp();
  }

  if (mParser->isSet("cli-help")) {
    Cli::showHelp();
    ::exit(EXIT_SUCCESS);
  }

  if (mParser->isSet("version"))
    mParser->showVersion();

  qInfo() << QStringLiteral("Use locale: %1").arg(mLocale);
}

App::~App () {
  qInfo() << QStringLiteral("Destroying app...");
  delete mEngine;
  delete mParser;
}

// -----------------------------------------------------------------------------

static QQuickWindow *createSubWindow (QQmlApplicationEngine *engine, const char *path) {
  QQmlComponent component(engine, QUrl(path));
  if (component.isError()) {
    qWarning() << component.errors();
	QString strErr;
	for each (QQmlError var in component.errors())
	{
		strErr = var.toString();
	}
    abort();
  }

  QObject *object = component.create();
  QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership);
  object->setParent(engine);

  return qobject_cast<QQuickWindow *>(object);
}

// -----------------------------------------------------------------------------

static void activeSplashScreen (QQmlApplicationEngine *engine) {
  qInfo() << QStringLiteral("Open splash screen...");
  QQuickWindow *splashScreen = ::createSubWindow(engine, cQmlViewSplashScreen);
  QObject::connect(CoreManager::getInstance()->getHandlers().get(), &CoreHandlers::coreStarted, splashScreen, [splashScreen] {
    splashScreen->close();
    splashScreen->deleteLater();
  });
}

void App::initContentApp () {
  shared_ptr<linphone::Config> config = ::getConfigIfExists(*mParser);
  bool mustBeIconified = false;

  // Destroy qml components and linphone core if necessary.
  if (mEngine) {
    qInfo() << QStringLiteral("Restarting app...");
    delete mEngine;

    mCallsWindow = nullptr;
    mSettingsWindow = nullptr;
    mNotifier = nullptr;
    mColors = nullptr;
    mSystemTrayIcon = nullptr;

    CoreManager::uninit();

    initLocale(config);
  } else {
    // Don't quit if last window is closed!!!
    setQuitOnLastWindowClosed(false);

    // Deal with received messages and CLI.
    QObject::connect(this, &App::receivedMessage, this, [](int, const QByteArray &byteArray) {
      QString command(byteArray);
      qInfo() << QStringLiteral("Received command from other application: `%1`.").arg(command);
      Cli::executeCommand(command);
    });

    // Add plugins directory.
    addLibraryPath(::Utils::coreStringToAppString(Paths::getPluginsDirPath()));
    qInfo() << QStringLiteral("Library paths:") << libraryPaths();

    mustBeIconified = mParser->isSet("iconified");
  }

  // Init core.
  CoreManager::init(this, mParser->value("config"));

  // Execute command argument if needed.
  if (!mEngine) {
    const QString commandArgument = getCommandArgument();
    if (!commandArgument.isEmpty()) {
      Cli::CommandFormat format;
      Cli::executeCommand(commandArgument, &format);
      if (format == Cli::UriFormat)
        mustBeIconified = true;
    }
  }

  // Init engine content.
  mEngine = new QQmlApplicationEngine();

  // Provide `+custom` folders for custom components and `5.9` for old components.
  // TODO: Remove 5.9 support in 6 months. (~ July 2018).
  {
    QStringList selectors("custom");
    const QVersionNumber &version = QLibraryInfo::version();
    if (version.majorVersion() == 5 && version.minorVersion() == 9)
      selectors.push_back("5.9");
    (new QQmlFileSelector(mEngine, mEngine))->setExtraSelectors(selectors);
  }
  qInfo() << QStringLiteral("Activated selectors:") << QQmlFileSelector::get(mEngine)->selector()->allSelectors();

  // Set modules paths.
  mEngine->addImportPath(":/ui/modules");
  mEngine->addImportPath(":/ui/scripts");
  mEngine->addImportPath(":/ui/views");
  mEngine->addImportPath("D:\\qtmsvc\\5.8\\msvc2015\\qml");
  // Provide avatars/thumbnails providers.
  mEngine->addImageProvider(AvatarProvider::PROVIDER_ID, new AvatarProvider());
  mEngine->addImageProvider(ImageProvider::PROVIDER_ID, new ImageProvider());
  mEngine->addImageProvider(ThumbnailProvider::PROVIDER_ID, new ThumbnailProvider());

  mColors = new Colors(this);
  mColors->useConfig(config);

  registerTypes();
  registerSharedTypes();
  registerToolTypes();
  registerSharedToolTypes();

  // Enable notifications.
  mNotifier = new Notifier(mEngine);

  // Load splashscreen.
  #ifdef Q_OS_MACOS
    ::activeSplashScreen(mEngine);
  #else
    if (!mustBeIconified)
      ::activeSplashScreen(mEngine);
  #endif // ifdef Q_OS_MACOS

  // Load main view.
  qInfo() << QStringLiteral("Loading main view...");
  mEngine->load(QUrl(cQmlViewMainWindow));
  if (mEngine->rootObjects().isEmpty())
  {
	  qFatal("Unable to open main window.");
  }
  QObject::connect(CoreManager::getInstance()->getHandlers().get(),
    &CoreHandlers::coreStarted, [this, mustBeIconified]() {
      openAppAfterInit(mustBeIconified);
    });
}

// -----------------------------------------------------------------------------

QString App::getCommandArgument () {
  const QStringList &arguments = mParser->positionalArguments();
  return arguments.empty() ? QString("") : arguments[0];
}

// -----------------------------------------------------------------------------

#ifdef Q_OS_MACOS

  bool App::event (QEvent *event) {
    if (event->type() == QEvent::FileOpen) {
      const QString url = static_cast<QFileOpenEvent *>(event)->url().toString();
      if (isSecondary()) {
        sendMessage(url.toLocal8Bit(), -1);
        ::exit(EXIT_SUCCESS);
      }

      Cli::executeCommand(url);
    }

    return SingleApplication::event(event);
  }

#endif // ifdef Q_OS_MACOS

// -----------------------------------------------------------------------------

QQuickWindow *App::getCallsWindow () {
  if (!mCallsWindow)
    mCallsWindow = ::createSubWindow(mEngine, cQmlViewCallsWindow);

  return mCallsWindow;
}

QQuickWindow *App::getMainWindow () const {
  return qobject_cast<QQuickWindow *>(
    const_cast<QQmlApplicationEngine *>(mEngine)->rootObjects().at(0)
  );
}

QQuickWindow *App::getSettingsWindow () {
  if (!mSettingsWindow) {
    mSettingsWindow = ::createSubWindow(mEngine, cQmlViewSettingsWindow);
    QObject::connect(mSettingsWindow, &QWindow::visibilityChanged, this, [](QWindow::Visibility visibility) {
      if (visibility == QWindow::Hidden) {
        qInfo() << QStringLiteral("Update nat policy.");
        shared_ptr<linphone::Core> core = CoreManager::getInstance()->getCore();
        core->setNatPolicy(core->getNatPolicy());
      }
    });
  }

  return mSettingsWindow;
}

// -----------------------------------------------------------------------------

void App::smartShowWindow (QQuickWindow *window) {
  window->setVisible(true);

  if (window->visibility() == QWindow::Minimized)
    window->show();

  window->raise();
  window->requestActivate();
}

// -----------------------------------------------------------------------------

bool App::hasFocus () const {
  return getMainWindow()->isActive() || (mCallsWindow && mCallsWindow->isActive());
}

// -----------------------------------------------------------------------------

void App::createParser () {
  delete mParser;

  mParser = new QCommandLineParser();
  mParser->setApplicationDescription(tr("applicationDescription"));
  mParser->addPositionalArgument("command", tr("commandLineDescription"), "[command]");
  mParser->addOptions({
    { { "h", "help" }, tr("commandLineOptionHelp") },
    { "cli-help", tr("commandLineOptionCliHelp") },
    { { "v", "version" }, tr("commandLineOptionVersion") },
    { "config", tr("commandLineOptionConfig"), tr("commandLineOptionConfigArg") },
    #ifndef Q_OS_MACOS
      { "iconified", tr("commandLineOptionIconified") },
    #endif // ifndef Q_OS_MACOS
    { { "V", "verbose" }, tr("commandLineOptionVerbose") }
  });
}

// -----------------------------------------------------------------------------

#define registerSharedSingletonType(TYPE, NAME, METHOD) qmlRegisterSingletonType<TYPE>( \
  "Linphone", 1, 0, NAME, \
  [](QQmlEngine *, QJSEngine *) -> QObject *{ \
    QObject *object = METHOD(); \
    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership); \
    return object; \
  } \
)

#define registerUncreatableType(TYPE, NAME) qmlRegisterUncreatableType<TYPE>( \
  "Linphone", 1, 0, NAME, NAME " is uncreatable." \
)

template<class T>
void registerSingletonType (const char *name) {
  qmlRegisterSingletonType<T>("Linphone", 1, 0, name, [](QQmlEngine *engine, QJSEngine *) -> QObject *{
      return new T(engine);
    });
}

template<class T>
void registerType (const char *name) {
  qmlRegisterType<T>("Linphone", 1, 0, name);
}

template<class T>
void registerToolType (const char *name) {
  qmlRegisterSingletonType<T>(name, 1, 0, name, [](QQmlEngine *engine, QJSEngine *) -> QObject *{
    return new T(engine);
  });
}

#define registerSharedToolType(TYPE, NAME, METHOD) qmlRegisterSingletonType<TYPE>( \
  NAME, 1, 0, NAME, \
  [](QQmlEngine *, QJSEngine *) ->  QObject *{ \
    QObject *object = const_cast<TYPE *>(METHOD()); \
    QQmlEngine::setObjectOwnership(object, QQmlEngine::CppOwnership); \
    return object; \
  } \
)

void App::registerTypes () {
  qInfo() << QStringLiteral("Registering types...");

  qRegisterMetaType<std::shared_ptr<linphone::ProxyConfig> >();
  qRegisterMetaType<ChatModel::EntryType>();

  registerType<AssistantModel>("AssistantModel");
  registerType<AuthenticationNotifier>("AuthenticationNotifier");
  registerType<CallsListProxyModel>("CallsListProxyModel");
  registerType<Camera>("Camera");
  registerType<CameraPreview>("CameraPreview");
  registerType<ChatProxyModel>("ChatProxyModel");
  registerType<ConferenceHelperModel>("ConferenceHelperModel");
  registerType<ConferenceModel>("ConferenceModel");
  registerType<ContactsListProxyModel>("ContactsListProxyModel");
  registerType<FileExtractor>("FileExtractor");
  registerType<SipAddressesProxyModel>("SipAddressesProxyModel");
  registerType<SoundPlayer>("SoundPlayer");
  registerType<TelephoneNumbersModel>("TelephoneNumbersModel");

  registerSingletonType<AudioCodecsModel>("AudioCodecsModel");
  registerSingletonType<OwnPresenceModel>("OwnPresenceModel");
  registerSingletonType<Presence>("Presence");
  registerSingletonType<TimelineModel>("TimelineModel");
  registerSingletonType<UrlHandlers>("UrlHandlers");
  registerSingletonType<VideoCodecsModel>("VideoCodecsModel");

  registerUncreatableType(CallModel, "CallModel");
  registerUncreatableType(ChatModel, "ChatModel");
  registerUncreatableType(ConferenceHelperModel::ConferenceAddModel, "ConferenceAddModel");
  registerUncreatableType(ContactModel, "ContactModel");
  registerUncreatableType(SipAddressObserver, "SipAddressObserver");
  registerUncreatableType(VcardModel, "VcardModel");
}

void App::registerSharedTypes () {
  qInfo() << QStringLiteral("Registering shared types...");

  registerSharedSingletonType(App, "App", App::getInstance);
  registerSharedSingletonType(CoreManager, "CoreManager", CoreManager::getInstance);
  registerSharedSingletonType(SettingsModel, "SettingsModel", CoreManager::getInstance()->getSettingsModel);
  registerSharedSingletonType(AccountSettingsModel, "AccountSettingsModel", CoreManager::getInstance()->getAccountSettingsModel);
  registerSharedSingletonType(SipAddressesModel, "SipAddressesModel", CoreManager::getInstance()->getSipAddressesModel);
  registerSharedSingletonType(CallsListModel, "CallsListModel", CoreManager::getInstance()->getCallsListModel);
  registerSharedSingletonType(ContactsListModel, "ContactsListModel", CoreManager::getInstance()->getContactsListModel);
}

void App::registerToolTypes () {
  qInfo() << QStringLiteral("Registering tool types...");

  registerToolType<Clipboard>("Clipboard");
  registerToolType<TextToSpeech>("TextToSpeech");
  registerToolType<Units>("Units");
}

void App::registerSharedToolTypes () {
  qInfo() << QStringLiteral("Registering shared tool types...");

  registerSharedToolType(Colors, "Colors", App::getInstance()->getColors);
}

#undef registerUncreatableType
#undef registerSharedToolType
#undef registerSharedSingletonType

// -----------------------------------------------------------------------------

void App::setTrayIcon () {
  QQuickWindow *root = getMainWindow();
  QSystemTrayIcon *systemTrayIcon = new QSystemTrayIcon(mEngine);

  // trayIcon: Right click actions.
  QAction *quitAction = new QAction("Quit", root);
  root->connect(quitAction, &QAction::triggered, this, &App::quit);

  QAction *restoreAction = new QAction("Restore", root);
  root->connect(restoreAction, &QAction::triggered, root, [root] {
    smartShowWindow(root);
  });

  // trayIcon: Left click actions.
  QMenu *menu = new QMenu();
  root->connect(systemTrayIcon, &QSystemTrayIcon::activated, [root](
    QSystemTrayIcon::ActivationReason reason
  ) {
    if (reason == QSystemTrayIcon::Trigger) {
      if (root->visibility() == QWindow::Hidden)
        smartShowWindow(root);
      else
        root->hide();
      }
  });

  // Build trayIcon menu.
  menu->addAction(restoreAction);
  menu->addSeparator();
  menu->addAction(quitAction);

  systemTrayIcon->setContextMenu(menu);
  systemTrayIcon->setIcon(QIcon(WINDOW_ICON_PATH));
  systemTrayIcon->setToolTip("Linphone");
  systemTrayIcon->show();

  mSystemTrayIcon = systemTrayIcon;
}

// -----------------------------------------------------------------------------

void App::initLocale (const shared_ptr<linphone::Config> &config) {
  // Try to use preferred locale.
  QString locale;
  if (config)
    locale = ::Utils::coreStringToAppString(config->getString(SettingsModel::UI_SECTION, "locale", ""));

  if (!locale.isEmpty() && ::installLocale(*this, *mTranslator, QLocale(locale))) {
    mLocale = locale;
    return;
  }

  // Try to use system locale.
  QLocale sysLocale = QLocale::system();
  if (::installLocale(*this, *mTranslator, sysLocale)) {
    mLocale = sysLocale.name();
    return;
  }

  // Use english.
  mLocale = cDefaultLocale;
  if (!::installLocale(*this, *mTranslator, QLocale(mLocale)))
    qFatal("Unable to install default translator.");
}

QString App::getConfigLocale () const {
  return ::Utils::coreStringToAppString(
    CoreManager::getInstance()->getCore()->getConfig()->getString(
      SettingsModel::UI_SECTION, "locale", ""
    )
  );
}

void App::setConfigLocale (const QString &locale) {
  CoreManager::getInstance()->getCore()->getConfig()->setString(
    SettingsModel::UI_SECTION, "locale", ::Utils::appStringToCoreString(locale)
  );

  emit configLocaleChanged(locale);
}

QString App::getLocale () const {
  return mLocale;
}

// -----------------------------------------------------------------------------

void App::openAppAfterInit (bool mustBeIconified) {
  qInfo() << QStringLiteral("Open linphone app.");

  QQuickWindow *mainWindow = getMainWindow();

  #ifndef __APPLE__
    // Enable TrayIconSystem.
    if (!QSystemTrayIcon::isSystemTrayAvailable())
      qWarning("System tray not found on this system.");
    else
      setTrayIcon();

    if (!mustBeIconified)
      smartShowWindow(mainWindow);
  #else
    smartShowWindow(mainWindow);
  #endif // ifndef __APPLE__

  // Display Assistant if it's the first time app launch.
  {
    shared_ptr<linphone::Config> config = CoreManager::getInstance()->getCore()->getConfig();
    if (config->getInt(SettingsModel::UI_SECTION, "force_assistant_at_startup", 1)) {
      QMetaObject::invokeMethod(mainWindow, "setView", Q_ARG(QVariant, "Assistant"), Q_ARG(QVariant, QString("")));
      config->setInt(SettingsModel::UI_SECTION, "force_assistant_at_startup", 0);
    }
  }

  #ifdef ENABLE_UPDATE_CHECK
    QTimer *timer = new QTimer(mEngine);
    timer->setInterval(cVersionUpdateCheckInterval);

    QObject::connect(timer, &QTimer::timeout, this, &App::checkForUpdate);
    timer->start();

    checkForUpdate();
  #endif // ifdef ENABLE_UPDATE_CHECK
}

// -----------------------------------------------------------------------------

void App::checkForUpdate () {
  CoreManager::getInstance()->getCore()->checkForUpdate(
    ::Utils::appStringToCoreString(applicationVersion())
  );
}
