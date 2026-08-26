// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "WindowsSmtcManager.h"

#include <QDebug>
#include <QMetaObject>
#include <QWindow>

// SMTC 互操作走直连 WinRT ABI（不使用 WRL），因此 MSVC 与 MinGW-w64 都可编译。
// 这里自声明了需要用到的一小段 Windows.Media 接口，只依赖系统自带的
// inspectable.h / roapi.h / winstring.h，避免依赖不同工具链里
// windows.media.h 是否存在及其 ABI 投影差异。
#if defined(Q_OS_WIN)
#  define QUEMUSIC_SMTC_IMPL 1
#else
#  define QUEMUSIC_SMTC_IMPL 0
#endif

#if QUEMUSIC_SMTC_IMPL

#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif

#  include <windows.h>
#  include <inspectable.h>
#  include <roapi.h>
#  include <winstring.h>

#  include <atomic>
#  include <cstring>
#  include <cwchar>

namespace SmtcAbi {

// WinRT 基础类型（二进制 ABI 兼容定义，字段名/内存布局与 SDK 头一致）
typedef unsigned char boolean;

struct EventRegistrationToken {
    INT64 value;
};

struct TimeSpan {
    INT64 Duration;
};

enum MediaPlaybackStatus {
    MediaPlaybackStatus_Closed   = 0,
    MediaPlaybackStatus_Changing = 1,
    MediaPlaybackStatus_Stopped  = 2,
    MediaPlaybackStatus_Playing  = 3,
    MediaPlaybackStatus_Paused   = 4
};

enum MediaPlaybackType {
    MediaPlaybackType_Unknown = 0,
    MediaPlaybackType_Music   = 1,
    MediaPlaybackType_Video   = 2,
    MediaPlaybackType_Image   = 3
};

enum SystemMediaTransportControlsButton {
    SystemMediaTransportControlsButton_Play        = 0,
    SystemMediaTransportControlsButton_Pause       = 1,
    SystemMediaTransportControlsButton_Stop        = 2,
    SystemMediaTransportControlsButton_Record      = 3,
    SystemMediaTransportControlsButton_FastForward = 4,
    SystemMediaTransportControlsButton_Rewind      = 5,
    SystemMediaTransportControlsButton_Next        = 6,
    SystemMediaTransportControlsButton_Previous    = 7,
    SystemMediaTransportControlsButton_ChannelUp   = 8,
    SystemMediaTransportControlsButton_ChannelDown = 9
};

// 只出现在 vtable 中但我们不调用其方法的占位类型
enum SoundLevel : int {
    SoundLevel_Muted = 0,
    SoundLevel_Low   = 1,
    SoundLevel_Full  = 2
};
enum MediaPlaybackAutoRepeatMode : int {
    MediaPlaybackAutoRepeatMode_None  = 0,
    MediaPlaybackAutoRepeatMode_Track = 1,
    MediaPlaybackAutoRepeatMode_List  = 2
};

struct IAsyncOperationBase; // 占位，实际签名只用到指针
template <typename TResult>
struct IAsyncOperation;
template <typename T>
struct IVector;

struct ISystemMediaTransportControls;
struct ISystemMediaTransportControls2;
struct ISystemMediaTransportControlsDisplayUpdater;
struct ISystemMediaTransportControlsTimelineProperties;
struct ISystemMediaTransportControlsButtonPressedEventArgs;
struct IPlaybackPositionChangeRequestedEventArgs;
struct IMusicDisplayProperties;
struct IMusicDisplayProperties2;
struct ISystemMediaTransportControlsInterop;

struct IAutoRepeatModeChangeRequestedEventArgs;
struct IPlaybackRateChangeRequestedEventArgs;
struct IShuffleEnabledChangeRequestedEventArgs;
struct IRandomAccessStreamReference;
struct IUriRuntimeClass;
struct IUriRuntimeClassFactory;
struct IRandomAccessStream;
struct IRandomAccessStreamWithContentType;
struct IWwwFormUrlDecoderRuntimeClass;
struct IVideoDisplayProperties;
struct IImageDisplayProperties;
struct IStorageFile;

// 事件处理器模板：WinRT 委托只有 IUnknown + Invoke
template <typename TSender, typename TArgs>
struct ITypedEventHandler : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE Invoke(TSender sender, TArgs args) = 0;
};

using ButtonPressedHandler = ITypedEventHandler<ISystemMediaTransportControls *,
                                                ISystemMediaTransportControlsButtonPressedEventArgs *>;
using PlaybackPositionHandler = ITypedEventHandler<ISystemMediaTransportControls *,
                                                   IPlaybackPositionChangeRequestedEventArgs *>;

// 接口/委托 IID（与 MIDL 生成值完全一致）
static const GUID IID_IUnknown = { 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
static const GUID IID_ISystemMediaTransportControls = { 0x99fa3ff4, 0x1742, 0x42a6, { 0x90, 0x2e, 0x08, 0x7d, 0x41, 0xf9, 0x65, 0xec } };
static const GUID IID_ISystemMediaTransportControls2 = { 0xea98d2f6, 0x7f3c, 0x4af2, { 0xa5, 0x86, 0x72, 0x88, 0x98, 0x08, 0xef, 0xb1 } };
static const GUID IID_ISystemMediaTransportControlsDisplayUpdater = { 0x8abbc53e, 0xfa55, 0x4ecf, { 0xad, 0x8e, 0xc9, 0x84, 0xe5, 0xdd, 0x15, 0x50 } };
static const GUID IID_ISystemMediaTransportControlsTimelineProperties = { 0x5125316a, 0xc3a2, 0x475b, { 0x85, 0x07, 0x93, 0x53, 0x4d, 0xc8, 0x8f, 0x15 } };
static const GUID IID_ISystemMediaTransportControlsButtonPressedEventArgs = { 0xb7f47116, 0xa56f, 0x4dc8, { 0x9e, 0x11, 0x92, 0x03, 0x1f, 0x4a, 0x87, 0xc2 } };
static const GUID IID_IPlaybackPositionChangeRequestedEventArgs = { 0xb4493f88, 0xeb28, 0x4961, { 0x9c, 0x14, 0x33, 0x5e, 0x44, 0xf3, 0xe1, 0x25 } };
static const GUID IID_IMusicDisplayProperties = { 0x6bbf0c59, 0xd0a0, 0x4d26, { 0x92, 0xa0, 0xf9, 0x78, 0xe1, 0xd1, 0x8e, 0x7b } };
static const GUID IID_IMusicDisplayProperties2 = { 0x00368462, 0x97d3, 0x44b9, { 0xb0, 0x0f, 0x00, 0x8a, 0xfc, 0xef, 0xaf, 0x18 } };
static const GUID IID_ISystemMediaTransportControlsInterop = { 0xddb0472d, 0xc911, 0x4a1f, { 0x86, 0xd9, 0xdc, 0x3d, 0x71, 0xa9, 0x5f, 0x5a } };
static const GUID IID_ITypedEventHandler_ButtonPressed = { 0x0557e996, 0x7b23, 0x5bae, { 0xaa, 0x81, 0xea, 0x0d, 0x67, 0x11, 0x43, 0xa4 } };
static const GUID IID_ITypedEventHandler_PlaybackPosition = { 0x44e34f15, 0xbdc0, 0x50a7, { 0xac, 0xe4, 0x39, 0xe9, 0x1f, 0xb7, 0x53, 0xf1 } };
// Windows.Storage.Streams.IRandomAccessStreamReference
static const GUID IID_IRandomAccessStreamReference = { 0x33ee3134, 0x1dd6, 0x4e3a, { 0x80, 0x67, 0xd1, 0xc1, 0x62, 0xe8, 0x64, 0x2b } };
// Windows.Storage.Streams.IRandomAccessStreamReferenceStatics
static const GUID IID_IRandomAccessStreamReferenceStatics = { 0x857309dc, 0x3fbf, 0x4e7d, { 0x98, 0x6f, 0xef, 0x3b, 0x1a, 0x07, 0xa9, 0x64 } };
// Windows.Foundation.IUriRuntimeClass
static const GUID IID_IUriRuntimeClass = { 0x9e365e57, 0x48b2, 0x4160, { 0x95, 0x6f, 0xc7, 0x38, 0x51, 0x20, 0xbb, 0xfc } };
// Windows.Foundation.IUriRuntimeClassFactory
static const GUID IID_IUriRuntimeClassFactory = { 0x44a9796f, 0x723e, 0x4fdf, { 0xa2, 0x18, 0x03, 0x3e, 0x75, 0xb0, 0xc0, 0x84 } };

inline bool guidEquals(const GUID &a, const GUID &b)
{
    return a.Data1 == b.Data1 && a.Data2 == b.Data2 && a.Data3 == b.Data3
        && std::memcmp(a.Data4, b.Data4, sizeof(a.Data4)) == 0;
}

// ---- 最小 ComPtr / HString ----

template <typename T>
class ComPtr
{
public:
    ComPtr() = default;
    ~ComPtr() { reset(); }

    ComPtr(const ComPtr &) = delete;
    ComPtr &operator=(const ComPtr &) = delete;

    ComPtr(ComPtr &&other) noexcept : m_ptr(other.m_ptr) { other.m_ptr = nullptr; }
    ComPtr &operator=(ComPtr &&other) noexcept
    {
        if (this != &other) {
            reset();
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    T *get() const { return m_ptr; }
    T **put() { reset(); return &m_ptr; }
    void attach(T *ptr) { reset(); m_ptr = ptr; }
    T *detach() { T *p = m_ptr; m_ptr = nullptr; return p; }
    void reset()
    {
        if (m_ptr) {
            m_ptr->Release();
            m_ptr = nullptr;
        }
    }

    explicit operator bool() const { return m_ptr != nullptr; }
    T *operator->() const { return m_ptr; }

private:
    T *m_ptr = nullptr;
};

class HString
{
public:
    HString() = default;
    ~HString() { clear(); }

    HString(const HString &) = delete;
    HString &operator=(const HString &) = delete;

    HString(HString &&other) noexcept : m_h(other.m_h) { other.m_h = nullptr; }
    HString &operator=(HString &&other) noexcept
    {
        if (this != &other) {
            clear();
            m_h = other.m_h;
            other.m_h = nullptr;
        }
        return *this;
    }

    static HString make(PCWSTR str, UINT32 length)
    {
        HString h;
        const PCWSTR source = (str && length > 0) ? str : L"";
        const UINT32 len = (str && length > 0) ? length : 0;
        WindowsCreateString(source, len, &h.m_h);
        return h;
    }

    bool isValid() const { return m_h != nullptr; }
    HSTRING get() const { return m_h; }
    void clear()
    {
        if (m_h) {
            WindowsDeleteString(m_h);
            m_h = nullptr;
        }
    }

private:
    HSTRING m_h = nullptr;
};

// 用于类名字符串等静态文本，避免每次分配
class HStringReference
{
public:
    explicit HStringReference(PCWSTR str)
    {
        const UINT32 len = str ? UINT32(wcslen(str)) : 0;
        if (str && len > 0)
            WindowsCreateStringReference(str, len, &m_header, &m_h);
    }

    bool isValid() const { return m_h != nullptr; }
    HSTRING get() const { return m_h; }

private:
    HSTRING_HEADER m_header = {};
    HSTRING m_h = nullptr;
};

// ---- WinRT ABI 接口（按 MIDL vtable 顺序声明） ----

struct ISystemMediaTransportControlsInterop : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE GetForWindow(HWND appWindow, REFIID riid, void **mediaTransportControl) = 0;
};

struct ISystemMediaTransportControls : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE get_PlaybackStatus(MediaPlaybackStatus *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_PlaybackStatus(MediaPlaybackStatus value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_DisplayUpdater(ISystemMediaTransportControlsDisplayUpdater **value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_SoundLevel(SoundLevel *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsEnabled(boolean *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_IsEnabled(boolean value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsPlayEnabled(boolean *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_IsPlayEnabled(boolean value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsStopEnabled(boolean *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_IsStopEnabled(boolean value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsPauseEnabled(boolean *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_IsPauseEnabled(boolean value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsRecordEnabled(boolean *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_IsRecordEnabled(boolean value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsFastForwardEnabled(boolean *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_IsFastForwardEnabled(boolean value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsRewindEnabled(boolean *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_IsRewindEnabled(boolean value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsPreviousEnabled(boolean *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_IsPreviousEnabled(boolean value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsNextEnabled(boolean *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_IsNextEnabled(boolean value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsChannelUpEnabled(boolean *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_IsChannelUpEnabled(boolean value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsChannelDownEnabled(boolean *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_IsChannelDownEnabled(boolean value) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_ButtonPressed(ButtonPressedHandler *handler, EventRegistrationToken *token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_ButtonPressed(EventRegistrationToken token) = 0;
};

struct ISystemMediaTransportControls2 : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE get_AutoRepeatMode(MediaPlaybackAutoRepeatMode *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_AutoRepeatMode(MediaPlaybackAutoRepeatMode value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_ShuffleEnabled(boolean *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_ShuffleEnabled(boolean value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_PlaybackRate(DOUBLE *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_PlaybackRate(DOUBLE value) = 0;
    virtual HRESULT STDMETHODCALLTYPE UpdateTimelineProperties(ISystemMediaTransportControlsTimelineProperties *timelineProperties) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_PlaybackPositionChangeRequested(PlaybackPositionHandler *handler, EventRegistrationToken *token) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_PlaybackPositionChangeRequested(EventRegistrationToken token) = 0;
};

struct ISystemMediaTransportControlsDisplayUpdater : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE get_Type(MediaPlaybackType *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_Type(MediaPlaybackType value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_AppMediaId(HSTRING *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_AppMediaId(HSTRING value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Thumbnail(IRandomAccessStreamReference **value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_Thumbnail(IRandomAccessStreamReference *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_MusicProperties(IMusicDisplayProperties **value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_VideoProperties(IVideoDisplayProperties **value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_ImageProperties(IImageDisplayProperties **value) = 0;
    virtual HRESULT STDMETHODCALLTYPE CopyFromFileAsync(MediaPlaybackType type, IStorageFile *source, IAsyncOperation<boolean> **operation) = 0;
    virtual HRESULT STDMETHODCALLTYPE ClearAll() = 0;
    virtual HRESULT STDMETHODCALLTYPE Update() = 0;
};

// Windows.Foundation.Uri（用于把封面 URL 转成 RandomAccessStreamReference）
struct IUriRuntimeClass : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE get_AbsoluteUri(HSTRING *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_DisplayUri(HSTRING *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Domain(HSTRING *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Extension(HSTRING *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Fragment(HSTRING *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Host(HSTRING *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Password(HSTRING *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Path(HSTRING *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Port(INT32 *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Query(HSTRING *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_QueryParsed(IWwwFormUrlDecoderRuntimeClass **value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_RawUri(HSTRING *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_SchemeName(HSTRING *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_UserName(HSTRING *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Suspicious(boolean *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE Equals(IUriRuntimeClass *pUri, boolean *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE CombineUri(PCWSTR relativeUri, IUriRuntimeClass **value) = 0;
};

struct IUriRuntimeClassFactory : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE CreateUri(HSTRING uri, IUriRuntimeClass **value) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateWithRelativeUri(HSTRING baseUri, HSTRING relativeUri, IUriRuntimeClass **value) = 0;
};

// Windows.Storage.Streams.IRandomAccessStreamReference（封面缩略图）
struct IRandomAccessStreamReference : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE OpenReadAsync(IAsyncOperation<IRandomAccessStreamWithContentType *> **operation) = 0;
};

// Windows.Storage.Streams.IRandomAccessStreamReferenceStatics（RandomAccessStreamReference 静态工厂）
struct IRandomAccessStreamReferenceStatics : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE CreateFromFile(IStorageFile *file, IRandomAccessStreamReference **value) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateFromUri(IUriRuntimeClass *uri, IRandomAccessStreamReference **value) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateFromStream(IRandomAccessStream *stream, IRandomAccessStreamReference **value) = 0;
};

struct IMusicDisplayProperties : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE get_Title(HSTRING *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_Title(HSTRING value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_AlbumArtist(HSTRING *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_AlbumArtist(HSTRING value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Artist(HSTRING *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_Artist(HSTRING value) = 0;
};

struct IMusicDisplayProperties2 : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE get_AlbumTitle(HSTRING *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_AlbumTitle(HSTRING value) = 0;
};

struct ISystemMediaTransportControlsTimelineProperties : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE get_StartTime(TimeSpan *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_StartTime(TimeSpan value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_EndTime(TimeSpan *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_EndTime(TimeSpan value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_MinSeekTime(TimeSpan *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_MinSeekTime(TimeSpan value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_MaxSeekTime(TimeSpan *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_MaxSeekTime(TimeSpan value) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Position(TimeSpan *value) = 0;
    virtual HRESULT STDMETHODCALLTYPE put_Position(TimeSpan value) = 0;
};

struct ISystemMediaTransportControlsButtonPressedEventArgs : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE get_Button(SystemMediaTransportControlsButton *value) = 0;
};

struct IPlaybackPositionChangeRequestedEventArgs : IInspectable {
    virtual HRESULT STDMETHODCALLTYPE get_RequestedPlaybackPosition(TimeSpan *value) = 0;
};

// 当前进程只有主窗口一个 SMTC 实例
static WindowsSmtcManager *s_smtcManager = nullptr;

static qint64 ticksToMs(INT64 ticks)
{
    return qint64(ticks / 10000); // 100ns → 1ms
}

static INT64 msToTicks(qint64 ms)
{
    return static_cast<INT64>(ms * 10000); // 1ms → 100ns
}

// 自实现 WinRT 委托对象：IUnknown + Invoke
template <typename TDerived, typename TInterface>
class CallbackObject : public TInterface
{
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override
    {
        if (!ppvObject)
            return E_POINTER;
        if (guidEquals(riid, TDerived::handlerIid()) || guidEquals(riid, IID_IUnknown)) {
            *ppvObject = static_cast<TInterface *>(this);
            AddRef();
            return S_OK;
        }
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override { return ++m_ref; }

    ULONG STDMETHODCALLTYPE Release() override
    {
        const ULONG ref = --m_ref;
        if (ref == 0)
            delete static_cast<TDerived *>(this);
        return ref;
    }

protected:
    std::atomic<ULONG> m_ref{ 1 };
};

class ButtonPressedCallback final
    : public CallbackObject<ButtonPressedCallback, ButtonPressedHandler>
{
public:
    static const GUID &handlerIid() { return IID_ITypedEventHandler_ButtonPressed; }

    HRESULT STDMETHODCALLTYPE Invoke(ISystemMediaTransportControls *,
                                     ISystemMediaTransportControlsButtonPressedEventArgs *args) override
    {
        if (args && s_smtcManager) {
            SystemMediaTransportControlsButton button;
            if (SUCCEEDED(args->get_Button(&button)))
                s_smtcManager->postButtonPressed(static_cast<int>(button));
        }
        return S_OK;
    }
};

class PlaybackPositionCallback final
    : public CallbackObject<PlaybackPositionCallback, PlaybackPositionHandler>
{
public:
    static const GUID &handlerIid() { return IID_ITypedEventHandler_PlaybackPosition; }

    HRESULT STDMETHODCALLTYPE Invoke(ISystemMediaTransportControls *,
                                     IPlaybackPositionChangeRequestedEventArgs *args) override
    {
        if (args && s_smtcManager) {
            TimeSpan position;
            if (SUCCEEDED(args->get_RequestedPlaybackPosition(&position)))
                s_smtcManager->postSeekRequested(ticksToMs(position.Duration));
        }
        return S_OK;
    }
};

// 把封面 URL（http/https/file）转成 RandomAccessStreamReference，
// 供 DisplayUpdater.Thumbnail 使用。失败或 scheme 不支持时返回空 ComPtr。
static ComPtr<IRandomAccessStreamReference> createThumbnailFromUrl(const QString &url)
{
    ComPtr<IRandomAccessStreamReference> result;
    if (url.isEmpty())
        return result;

    const QString trimmed = url.trimmed();
    if (trimmed.isEmpty())
        return result;

    // 只处理 SMTC 能真正读取的 scheme（http/https/file）。
    // qrc:/ 等 Qt 内部 scheme 对 Windows 而言无意义，塞给 SMTC 只会失败。
    const QString scheme = trimmed.section(QStringLiteral("://"), 0, 0).toLower();
    if (scheme != QStringLiteral("http") && scheme != QStringLiteral("https")
        && scheme != QStringLiteral("file")) {
        return result;
    }

    // 1) 创建 Windows.Foundation.Uri
    HStringReference uriClassId(L"Windows.Foundation.Uri");
    if (!uriClassId.isValid())
        return result;

    ComPtr<IUriRuntimeClassFactory> uriFactory;
    HRESULT hr = RoGetActivationFactory(uriClassId.get(), IID_IUriRuntimeClassFactory,
                                        reinterpret_cast<void **>(uriFactory.put()));
    if (FAILED(hr) || !uriFactory)
        return result;

    HString hUri = HString::make(reinterpret_cast<PCWSTR>(trimmed.utf16()),
                                 UINT32(trimmed.size()));
    if (!hUri.isValid())
        return result;

    ComPtr<IUriRuntimeClass> uri;
    hr = uriFactory->CreateUri(hUri.get(), uri.put());
    if (FAILED(hr) || !uri)
        return result;

    // 2) 通过 RandomAccessStreamReference 静态工厂 CreateFromUri
    HStringReference streamRefClassId(L"Windows.Storage.Streams.RandomAccessStreamReference");
    if (!streamRefClassId.isValid())
        return result;

    ComPtr<IRandomAccessStreamReferenceStatics> statics;
    hr = RoGetActivationFactory(streamRefClassId.get(), IID_IRandomAccessStreamReferenceStatics,
                                reinterpret_cast<void **>(statics.put()));
    if (FAILED(hr) || !statics)
        return result;

    hr = statics->CreateFromUri(uri.get(), result.put());
    if (FAILED(hr) || !result)
        result.reset();
    return result;
}

} // namespace SmtcAbi

class WindowsSmtcManager::Private
{
public:
    QWindow *window = nullptr;
    bool initialized = false;

    SmtcAbi::ComPtr<SmtcAbi::ISystemMediaTransportControls> smtc;
    SmtcAbi::ComPtr<SmtcAbi::ISystemMediaTransportControls2> smtc2;
    SmtcAbi::ComPtr<SmtcAbi::ISystemMediaTransportControlsDisplayUpdater> displayUpdater;

    SmtcAbi::ComPtr<SmtcAbi::ButtonPressedHandler> buttonHandler;
    SmtcAbi::ComPtr<SmtcAbi::PlaybackPositionHandler> seekHandler;
    SmtcAbi::EventRegistrationToken buttonToken = {};
    SmtcAbi::EventRegistrationToken seekToken = {};

    bool init(QWindow *window);
    void deinit();
    void setControlsEnabled(bool play, bool pause, bool next, bool previous);
    void setPlaybackStatus(int status);
    void updateMediaInfo(const QString &title, const QString &artist, const QString &album,
                         const QString &cover);
    void updateTimeline(qint64 positionMs, qint64 durationMs);
};

bool WindowsSmtcManager::Private::init(QWindow *window)
{
    if (initialized)
        return true;
    if (!window)
        return false;

    // 确保 native window 已创建，才能拿到 SMTC 所需的 HWND
    if (!window->handle())
        window->create();
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd) {
        qWarning() << "[SMTC] 无法获取主窗口句柄";
        return false;
    }

    SmtcAbi::HStringReference smtcClassId(L"Windows.Media.SystemMediaTransportControls");
    if (!smtcClassId.isValid()) {
        qWarning() << "[SMTC] 创建 SMTC 类名 HSTRING 失败";
        return false;
    }

    SmtcAbi::ComPtr<SmtcAbi::ISystemMediaTransportControlsInterop> interop;
    HRESULT hr = RoGetActivationFactory(smtcClassId.get(),
                                        SmtcAbi::IID_ISystemMediaTransportControlsInterop,
                                        reinterpret_cast<void **>(interop.put()));
    if (FAILED(hr) || !interop) {
        qWarning() << "[SMTC] 获取 SMTC Interop 工厂失败:" << Qt::hex << hr;
        return false;
    }

    hr = interop->GetForWindow(hwnd,
                               SmtcAbi::IID_ISystemMediaTransportControls,
                               reinterpret_cast<void **>(smtc.put()));
    if (FAILED(hr) || !smtc) {
        qWarning() << "[SMTC] GetForWindow 失败:" << Qt::hex << hr;
        return false;
    }

    // ISystemMediaTransportControls2 用于时间线（可选）
    smtc->QueryInterface(SmtcAbi::IID_ISystemMediaTransportControls2,
                         reinterpret_cast<void **>(smtc2.put()));

    hr = smtc->get_DisplayUpdater(displayUpdater.put());
    if (FAILED(hr) || !displayUpdater) {
        qWarning() << "[SMTC] 获取 DisplayUpdater 失败:" << Qt::hex << hr;
        smtc.reset();
        smtc2.reset();
        return false;
    }

    smtc->put_IsPlayEnabled(static_cast<SmtcAbi::boolean>(TRUE));
    smtc->put_IsPauseEnabled(static_cast<SmtcAbi::boolean>(TRUE));
    smtc->put_IsNextEnabled(static_cast<SmtcAbi::boolean>(TRUE));
    smtc->put_IsPreviousEnabled(static_cast<SmtcAbi::boolean>(TRUE));
    smtc->put_IsEnabled(static_cast<SmtcAbi::boolean>(TRUE));
    smtc->put_PlaybackStatus(SmtcAbi::MediaPlaybackStatus_Closed);

    buttonHandler.attach(new SmtcAbi::ButtonPressedCallback());
    hr = smtc->add_ButtonPressed(buttonHandler.get(), &buttonToken);
    if (FAILED(hr))
        qWarning() << "[SMTC] 注册按钮事件失败:" << Qt::hex << hr;

    if (smtc2) {
        seekHandler.attach(new SmtcAbi::PlaybackPositionCallback());
        hr = smtc2->add_PlaybackPositionChangeRequested(seekHandler.get(), &seekToken);
        if (FAILED(hr))
            qWarning() << "[SMTC] 注册时间线拖动事件失败:" << Qt::hex << hr;
    }

    initialized = true;
    return true;
}

void WindowsSmtcManager::Private::deinit()
{
    if (buttonToken.value != 0 && smtc) {
        smtc->remove_ButtonPressed(buttonToken);
        buttonToken = {};
    }
    if (seekToken.value != 0 && smtc2) {
        smtc2->remove_PlaybackPositionChangeRequested(seekToken);
        seekToken = {};
    }

    if (smtc) {
        smtc->put_PlaybackStatus(SmtcAbi::MediaPlaybackStatus_Closed);
        smtc->put_IsEnabled(static_cast<SmtcAbi::boolean>(FALSE));
    }

    buttonHandler.reset();
    seekHandler.reset();
    displayUpdater.reset();
    smtc2.reset();
    smtc.reset();
    window = nullptr;
    initialized = false;
}

void WindowsSmtcManager::Private::setControlsEnabled(bool play, bool pause, bool next,
                                                     bool previous)
{
    if (!initialized || !smtc)
        return;
    smtc->put_IsPlayEnabled(static_cast<SmtcAbi::boolean>(play ? TRUE : FALSE));
    smtc->put_IsPauseEnabled(static_cast<SmtcAbi::boolean>(pause ? TRUE : FALSE));
    smtc->put_IsPreviousEnabled(static_cast<SmtcAbi::boolean>(previous ? TRUE : FALSE));
    smtc->put_IsNextEnabled(static_cast<SmtcAbi::boolean>(next ? TRUE : FALSE));
}

void WindowsSmtcManager::Private::setPlaybackStatus(int status)
{
    if (!initialized || !smtc)
        return;

    SmtcAbi::MediaPlaybackStatus smtcStatus = SmtcAbi::MediaPlaybackStatus_Closed;
    switch (static_cast<WindowsSmtcManager::PlaybackStatus>(status)) {
    case WindowsSmtcManager::Playing:
        smtcStatus = SmtcAbi::MediaPlaybackStatus_Playing;
        break;
    case WindowsSmtcManager::Paused:
        smtcStatus = SmtcAbi::MediaPlaybackStatus_Paused;
        break;
    case WindowsSmtcManager::Stopped:
        smtcStatus = SmtcAbi::MediaPlaybackStatus_Stopped;
        break;
    case WindowsSmtcManager::Changing:
        smtcStatus = SmtcAbi::MediaPlaybackStatus_Changing;
        break;
    case WindowsSmtcManager::Closed:
    default:
        smtcStatus = SmtcAbi::MediaPlaybackStatus_Closed;
        break;
    }
    smtc->put_PlaybackStatus(smtcStatus);
}

void WindowsSmtcManager::Private::updateMediaInfo(const QString &title,
                                                  const QString &artist,
                                                  const QString &album,
                                                  const QString &cover)
{
    if (!initialized || !displayUpdater)
        return;

    const QString titleStr = title.isEmpty() ? QStringLiteral("QueMusic") : title;
    const QString artistStr = artist.isEmpty() ? QStringLiteral("未知歌手") : artist;

    displayUpdater->put_Type(SmtcAbi::MediaPlaybackType_Music);

    SmtcAbi::ComPtr<SmtcAbi::IMusicDisplayProperties> musicProps;
    if (FAILED(displayUpdater->get_MusicProperties(musicProps.put())) || !musicProps)
        return;

    {
        SmtcAbi::HString hTitle = SmtcAbi::HString::make(
            reinterpret_cast<PCWSTR>(titleStr.utf16()), UINT32(titleStr.size()));
        if (hTitle.isValid())
            musicProps->put_Title(hTitle.get());
    }
    {
        SmtcAbi::HString hArtist = SmtcAbi::HString::make(
            reinterpret_cast<PCWSTR>(artistStr.utf16()), UINT32(artistStr.size()));
        if (hArtist.isValid())
            musicProps->put_Artist(hArtist.get());
    }

    if (!album.isEmpty()) {
        SmtcAbi::ComPtr<SmtcAbi::IMusicDisplayProperties2> musicProps2;
        if (SUCCEEDED(musicProps->QueryInterface(SmtcAbi::IID_IMusicDisplayProperties2,
                                                 reinterpret_cast<void **>(musicProps2.put())))
            && musicProps2) {
            SmtcAbi::HString hAlbum = SmtcAbi::HString::make(
                reinterpret_cast<PCWSTR>(album.utf16()), UINT32(album.size()));
            if (hAlbum.isValid())
                musicProps2->put_AlbumTitle(hAlbum.get());
        }
    }

    // 封面缩略图（专辑封面）：SMTC 弹窗里显示的音乐图标
    if (!cover.isEmpty()) {
        SmtcAbi::ComPtr<SmtcAbi::IRandomAccessStreamReference> thumb =
            SmtcAbi::createThumbnailFromUrl(cover);
        if (thumb)
            displayUpdater->put_Thumbnail(thumb.get());
    }

    displayUpdater->Update();
}

void WindowsSmtcManager::Private::updateTimeline(qint64 positionMs, qint64 durationMs)
{
    if (!initialized || !smtc2)
        return;

    SmtcAbi::HStringReference timelineClassId(
        L"Windows.Media.SystemMediaTransportControlsTimelineProperties");
    if (!timelineClassId.isValid())
        return;

    SmtcAbi::ComPtr<IInspectable> instance;
    HRESULT hr = RoActivateInstance(timelineClassId.get(), instance.put());
    if (FAILED(hr) || !instance)
        return;

    SmtcAbi::ComPtr<SmtcAbi::ISystemMediaTransportControlsTimelineProperties> timeline;
    hr = instance->QueryInterface(SmtcAbi::IID_ISystemMediaTransportControlsTimelineProperties,
                                  reinterpret_cast<void **>(timeline.put()));
    if (FAILED(hr) || !timeline)
        return;

    const qint64 safePosition = qMax<qint64>(0, positionMs);
    const qint64 safeDuration = qMax<qint64>(0, durationMs);

    // 时长未知（如直播流或刚切歌的瞬间）时不设置时间线，否则
    // EndTime=0 / Position>0 会让系统媒体弹窗拒绝显示进度条。
    if (safeDuration <= 0)
        return;

    const qint64 clampedPosition = qMin(safePosition, safeDuration);

    SmtcAbi::TimeSpan zero;
    zero.Duration = 0;
    SmtcAbi::TimeSpan position;
    position.Duration = SmtcAbi::msToTicks(clampedPosition);
    SmtcAbi::TimeSpan end;
    end.Duration = SmtcAbi::msToTicks(safeDuration);

    timeline->put_StartTime(zero);
    timeline->put_EndTime(end);
    timeline->put_MinSeekTime(zero);
    timeline->put_MaxSeekTime(end);
    timeline->put_Position(position);

    smtc2->UpdateTimelineProperties(timeline.get());
}

#else // QUEMUSIC_SMTC_IMPL

class WindowsSmtcManager::Private
{
public:
    bool initialized = false;
};

#endif // QUEMUSIC_SMTC_IMPL

WindowsSmtcManager::WindowsSmtcManager(QObject *parent)
    : QObject(parent)
    , d(new Private)
{
}

WindowsSmtcManager::~WindowsSmtcManager()
{
#if QUEMUSIC_SMTC_IMPL
    if (SmtcAbi::s_smtcManager == this)
        SmtcAbi::s_smtcManager = nullptr;
    d->deinit();
#endif
    delete d;
}

bool WindowsSmtcManager::isAvailable() const
{
    return d->initialized;
}

void WindowsSmtcManager::initialize(QWindow *window)
{
#if QUEMUSIC_SMTC_IMPL
    if (d->initialized)
        return;
    if (!window) {
        qWarning() << "[SMTC] initialize() 需要传入主窗口";
        return;
    }

    SmtcAbi::s_smtcManager = this;
    if (!d->init(window)) {
        if (SmtcAbi::s_smtcManager == this)
            SmtcAbi::s_smtcManager = nullptr;
    }
    emit availableChanged();
#else
    Q_UNUSED(window);
    qWarning() << "[SMTC] 当前平台不支持 Windows SMTC";
#endif
}

void WindowsSmtcManager::shutdown()
{
#if QUEMUSIC_SMTC_IMPL
    if (SmtcAbi::s_smtcManager == this)
        SmtcAbi::s_smtcManager = nullptr;
    if (d->initialized)
        d->deinit();
    emit availableChanged();
#endif
}

void WindowsSmtcManager::setControlsEnabled(bool play, bool pause, bool next, bool previous)
{
#if QUEMUSIC_SMTC_IMPL
    d->setControlsEnabled(play, pause, next, previous);
#else
    Q_UNUSED(play); Q_UNUSED(pause); Q_UNUSED(next); Q_UNUSED(previous);
#endif
}

void WindowsSmtcManager::setPlaybackStatus(int status)
{
#if QUEMUSIC_SMTC_IMPL
    d->setPlaybackStatus(status);
#else
    Q_UNUSED(status);
#endif
}

void WindowsSmtcManager::updateMediaInfo(const QString &title, const QString &artist,
                                         const QString &album, const QString &cover)
{
#if QUEMUSIC_SMTC_IMPL
    d->updateMediaInfo(title, artist, album, cover);
#else
    Q_UNUSED(title); Q_UNUSED(artist); Q_UNUSED(album); Q_UNUSED(cover);
#endif
}

void WindowsSmtcManager::updateTimeline(qint64 positionMs, qint64 durationMs)
{
#if QUEMUSIC_SMTC_IMPL
    d->updateTimeline(positionMs, durationMs);
#else
    Q_UNUSED(positionMs); Q_UNUSED(durationMs);
#endif
}

void WindowsSmtcManager::postButtonPressed(int button)
{
#if QUEMUSIC_SMTC_IMPL
    // 从 WinRT 工作线程安全地投递到主线程
    QMetaObject::invokeMethod(this, [this, button] {
        using B = SmtcAbi::SystemMediaTransportControlsButton;
        switch (static_cast<B>(button)) {
        case SmtcAbi::SystemMediaTransportControlsButton_Play:
            emit playPressed();
            break;
        case SmtcAbi::SystemMediaTransportControlsButton_Pause:
            emit pausePressed();
            break;
        case SmtcAbi::SystemMediaTransportControlsButton_Next:
            emit nextPressed();
            break;
        case SmtcAbi::SystemMediaTransportControlsButton_Previous:
            emit previousPressed();
            break;
        default:
            break;
        }
    }, Qt::QueuedConnection);
#else
    Q_UNUSED(button);
#endif
}

void WindowsSmtcManager::postSeekRequested(qint64 positionMs)
{
#if QUEMUSIC_SMTC_IMPL
    QMetaObject::invokeMethod(this, [this, positionMs] {
        emit seekRequested(positionMs);
    }, Qt::QueuedConnection);
#else
    Q_UNUSED(positionMs);
#endif
}