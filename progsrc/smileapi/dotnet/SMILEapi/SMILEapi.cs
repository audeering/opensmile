/*F***************************************************************************
 * This file is part of openSMILE.
 *
 * Copyright (c) audEERING GmbH. All rights reserved.
 * See the file COPYING for details on license terms.
 ***************************************************************************E*/

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Text;

namespace AudEERING.openSMILE
{
    public enum OpenSmileResult
    {
        SMILE_SUCCESS,         
        SMILE_FAIL,          
        SMILE_INVALID_ARG,   
        SMILE_INVALID_STATE,   
        SMILE_COMP_NOT_FOUND,   
        SMILE_LICENSE_FAIL,    
        SMILE_CONFIG_PARSE_FAIL, 
        SMILE_CONFIG_INIT_FAIL,  
        SMILE_NOT_WRITTEN,      
    }

    public enum OpenSmileState
    {
        Uninitialized,
        Initialized,
        Running,
        //Paused,
        Ended
    }

    public enum OpenSmileLogType
    {
        Message = 1,
        Warning = 2,
        Error = 3,
        Debug = 4,
        Print = 5
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct OpenSmileLogMessage
    {
        public OpenSmileLogType Type;
        public int Level;
        public string Text;
        public string Module;

        static readonly string[] types = { "MSG", "WRN", "ERR", "DBG" };

        public override string ToString()
        {
            if (Type == OpenSmileLogType.Print)
                return Text;
            else
            {
                StringBuilder stringBuilder = new StringBuilder();
                
                stringBuilder.Append('(');
                stringBuilder.Append(types[(int)Type - 1]);
                stringBuilder.Append(") [");
                stringBuilder.Append(Level);
                stringBuilder.Append(']');

                if (Module != null) 
                {
                    stringBuilder.Append(' ');
                    stringBuilder.Append(Module);
                }                

                stringBuilder.Append(": ");
                stringBuilder.Append(Text);

                return stringBuilder.ToString();
            }
        }
    }

    public delegate void LogCallback(IntPtr smileobj, OpenSmileLogMessage message, IntPtr param);
    public delegate void StateChangedCallback(IntPtr smileobj, OpenSmileState state, IntPtr param);
    public delegate bool ExternalSinkCallback([MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 1)]float[] data, long vectorSize, IntPtr param);
    public delegate bool ExternalMessageInterfaceCallback(ComponentMessage msg, IntPtr param);
    public delegate bool ExternalMessageInterfaceJsonCallback(string msg, IntPtr param);

    [StructLayout(LayoutKind.Sequential)]
    internal struct smileopt_t
    {
        public string Name;
        public string Value;

        public smileopt_t(string name, string value)
        {
            Name = name;
            Value = value;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    public class ComponentMessage
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
        private char[] msgType;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 32)]
        private char[] msgName;

        public string Sender;
        public double SmileTime;
        public double UserTime1;
        public double UserTime2;
        public double ReaderTime;
        public int MsgID;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public double[] FloatData;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
        public int[] IntData;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 64)]
        private char[] msgText;

        public int UserFlag1;
        public int UserFlag2;
        public int UserFlag3;
        public IntPtr CustData;
        public IntPtr CustData2;
        public int CustDataSize;
        public int CustData2Size;
        public int CustDataType;
        public int CustData2Type;

        public string MsgType { get { return ParseNullTerminatedString(msgType); }}
        public string MsgName { get { return ParseNullTerminatedString(msgName); }}
        public string MsgText { get { return ParseNullTerminatedString(msgText); }}

        private static string ParseNullTerminatedString(char[] chars)
        {
            int end = Array.FindIndex(chars, c => c == '\0');
            if (end >= 0)
                return new string(chars, 0, end);
            else
                return new string(chars);
        }
    }

    internal static class OpenSmileApi
    {
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_new")]
        public static extern IntPtr Smile_new();
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_initialize")]
        public static extern OpenSmileResult Smile_initialize(IntPtr smileobj, string configFile, int nOptions, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex=1)] smileopt_t[] options, int loglevel, int debug, int consoleOutput, string logFile);
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_run")]
        public static extern OpenSmileResult Smile_run(IntPtr smileobj);
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_abort")]
        public static extern OpenSmileResult Smile_abort(IntPtr smileobj);
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_reset")]
        public static extern OpenSmileResult Smile_reset(IntPtr smileobj);
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_get_state")]
        public static extern OpenSmileState Smile_get_state(IntPtr smileobj);
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_set_log_callback")]
        public static extern OpenSmileResult Smile_set_log_callback(IntPtr smileobj, LogCallback callback, IntPtr param);
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_set_state_callback")]
        public static extern OpenSmileResult Smile_set_state_callback(IntPtr smileobj, StateChangedCallback callback, IntPtr param);
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_free")]
        public static extern void Smile_free(IntPtr smileobj);

        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_extsource_write_data")]
        public static extern OpenSmileResult Smile_extsource_write_data(IntPtr smileobj, string componentName, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 3)]float[,] data, int nFrames);
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_extsource_write_data")]
        public static extern OpenSmileResult Smile_extsource_write_data(IntPtr smileobj, string componentName, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 3)]double[,] data, int nFrames);
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_extsource_set_external_eoi")]
        public static extern OpenSmileResult Smile_extsource_set_external_eoi(IntPtr smileobj, string componentName);
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_extaudiosource_write_data")]
        public static extern OpenSmileResult Smile_extaudiosource_write_data(IntPtr smileobj, string componentName, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 3)]byte[] data, int length);
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_extaudiosource_write_data")]
        public static extern OpenSmileResult Smile_extaudiosource_write_data(IntPtr smileobj, string componentName, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 3)]short[] data, int length);
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_extaudiosource_write_data")]
        public static extern OpenSmileResult Smile_extaudiosource_write_data(IntPtr smileobj, string componentName, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 3)]float[] data, int length);
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_extaudiosource_write_data")]
        public static extern OpenSmileResult Smile_extaudiosource_write_data(IntPtr smileobj, string componentName, [MarshalAs(UnmanagedType.LPArray, SizeParamIndex = 3)]double[] data, int length);
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_extaudiosource_set_external_eoi")]
        public static extern OpenSmileResult Smile_extaudiosource_set_external_eoi(IntPtr smileobj, string componentName);
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_extsink_set_data_callback")]
        public static extern OpenSmileResult Smile_extsink_set_data_callback(IntPtr smileobj, string componentName, ExternalSinkCallback callback, IntPtr param);
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_extmsginterface_set_msg_callback")]
        public static extern OpenSmileResult Smile_extmsginterface_set_msg_callback(IntPtr smileobj, string componentName, ExternalMessageInterfaceCallback callback, IntPtr param);
        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_extmsginterface_set_json_msg_callback")]
        public static extern OpenSmileResult Smile_extmsginterface_set_json_msg_callback(IntPtr smileobj, string componentName, ExternalMessageInterfaceJsonCallback callback, IntPtr param);

        [DllImport("SMILEapi", CharSet = CharSet.Ansi, EntryPoint = "smile_error_msg")]
        public static extern string Smile_error_msg(IntPtr smileobj);
    }

    public class LogMessageEventArgs : EventArgs
    {
        public OpenSmileLogMessage Message { get; }

        public LogMessageEventArgs(OpenSmileLogMessage message)
        {
            Message = message;
        }
    }

    public class StateChangedEventArgs : EventArgs
    {
        public OpenSmileState State { get; }

        public StateChangedEventArgs(OpenSmileState state)
        {
            State = state;
        }
    }

    /// <summary> 
    /// The main class implementing the interface to OpenSMILE.   
    /// </summary>
    public class OpenSMILE : IDisposable
    {
        bool disposed = false;
        IntPtr smileObj;

        // we need to store these delegates as fields to prevent them from being garbage-collected
        LogCallback logCallback;
        StateChangedCallback stateCallback;

        /// <summary>
        /// Gets the current execution state of openSMILE.
        /// Note: since openSMILE runs in its own unsynchronized thread, the actual state may have
        /// changed in the mean-time and may be different than the value returned.
        /// </summary>
        /// <returns>
        /// The current execution state of openSMILE.
        /// </returns>
        /// <exception cref="ObjectDisposedException">
        /// Thrown if the object has already been disposed.
        /// </exception>
        public OpenSmileState State
        {
            get
            {
                if (disposed)
                    throw new ObjectDisposedException(typeof(OpenSMILE).FullName);
                if (smileObj == IntPtr.Zero)
                    return OpenSmileState.Uninitialized;
                return OpenSmileApi.Smile_get_state(smileObj);
            }
        }

        /// <summary>
        /// Event invoked for every log message generated by openSMILE.
        /// The logLevel and debug arguments passed to the Initialize method determine for which log message levels and types the event is invoked.
        /// </summary>
        public event EventHandler<LogMessageEventArgs> LogMessage;

        /// <summary>
        /// Event invoked whenever the openSMILE execution state changes.
        /// </summary>
        public event EventHandler<StateChangedEventArgs> StateChanged; 

        /// <summary>
        /// Initializes OpenSMILE with the provided config file and command-line options.
        /// </summary>
        /// <param name="configFile">Path to the config file to load.</param>
        /// <param name="options">Mapping of option names to values.</param>
        /// <param name="logLevel">Log level to set for the logger.</param>
        /// <param name="debug">Whether to enable debug mode (outputs debug-level log messages). SMILEapi must be compiled in debug mode for this to take effect.</param>
        /// <param name="consoleOutput">Whether log messages are printed to the console (useful for debugging).</param>
        /// <exception cref="ObjectDisposedException">
        /// Thrown if the object has already been disposed.
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown if openSMILE has already been initialized before.
        /// </exception>
        /// <exception cref="OpenSmileException">
        /// Thrown for internal openSMILE errors.
        /// </exception>
        public void Initialize(string configFile, Dictionary<string, string> options, int logLevel=2, bool debug=false, bool consoleOutput=false)
        {
            if (disposed)
                throw new ObjectDisposedException(typeof(OpenSMILE).FullName);
            if (smileObj != IntPtr.Zero)
                throw new InvalidOperationException("openSMILE has already been initialized before.");

            smileObj = OpenSmileApi.Smile_new();
            if (smileObj == IntPtr.Zero)
                throw new OpenSmileException(OpenSmileResult.SMILE_FAIL, "Could not create new SMILEapi object");

            logCallback = OnLogMessage;
            stateCallback = OnStateChanged;
            OpenSmileApi.Smile_set_log_callback(smileObj, logCallback, IntPtr.Zero);
            OpenSmileApi.Smile_set_state_callback(smileObj, stateCallback, IntPtr.Zero);

            smileopt_t[] opts = options?.Select(opt => new smileopt_t(opt.Key, opt.Value)).ToArray();

            CheckSmileResult(OpenSmileApi.Smile_initialize(smileObj, configFile, opts.Length, opts, logLevel, debug ? 1 : 0, consoleOutput ? 1 : 0, null));
        }

        private void OnLogMessage(IntPtr smileobj, OpenSmileLogMessage message, IntPtr param)
        {
            LogMessage?.Invoke(this, new LogMessageEventArgs(message));
        }

        private void OnStateChanged(IntPtr smileobj, OpenSmileState state, IntPtr param)
        {
            StateChanged?.Invoke(this, new StateChangedEventArgs(state));
        }

        /// <summary>
        /// Writes a data buffer to the specified instance of a cExternalSource component.
        /// </summary>
        /// <param name="componentName">Instance name of the cExternalSource component to write to.</param>
        /// <param name="data">Data frames to write.</param>
        /// <returns>
        /// Returns true if the data was written successfully, otherwise returns false
        /// (e.g. if the internal buffer of the component is full).
        /// </returns>
        /// <exception cref="ObjectDisposedException">
        /// Thrown if the object has already been disposed.
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown if openSMILE has not been initialized yet.
        /// </exception>
        /// <exception cref="OpenSmileException">
        /// Thrown for internal openSMILE errors.
        /// </exception>
        public bool ExternalSourceWriteData(string componentName, float[,] data)
        {
            if (disposed)
                throw new ObjectDisposedException(typeof(OpenSMILE).FullName);
            if (smileObj == IntPtr.Zero)
                throw new InvalidOperationException("openSMILE must be initialized first.");

            OpenSmileResult result = OpenSmileApi.Smile_extsource_write_data(smileObj, componentName, data, data.GetLength(0));

            if (result == OpenSmileResult.SMILE_SUCCESS)
                return true;
            else if (result == OpenSmileResult.SMILE_NOT_WRITTEN)
                return false;
            else
            {
                CheckSmileResult(result);
                throw new Exception(); // unreachable as CheckSmileResult will throw, just to make compiler happy
            }
        }

        /// <summary>
        /// Writes a data buffer to the specified instance of a cExternalSource component.
        /// </summary>
        /// <param name="componentName">Instance name of the cExternalSource component to write to.</param>
        /// <param name="data">Data frames to write.</param>
        /// <returns>
        /// Returns true if the data was written successfully, otherwise returns false
        /// (e.g. if the internal buffer of the component is full).
        /// </returns>
        /// <exception cref="ObjectDisposedException">
        /// Thrown if the object has already been disposed.
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown if openSMILE has not been initialized yet.
        /// </exception>
        /// <exception cref="OpenSmileException">
        /// Thrown for internal openSMILE errors.
        /// </exception>
        public bool ExternalSourceWriteData(string componentName, double[,] data)
        {
            if (disposed)
                throw new ObjectDisposedException(typeof(OpenSMILE).FullName);
            if (smileObj == IntPtr.Zero)
                throw new InvalidOperationException("openSMILE must be initialized first.");

            OpenSmileResult result = OpenSmileApi.Smile_extsource_write_data(smileObj, componentName, data, data.GetLength(0));

            if (result == OpenSmileResult.SMILE_SUCCESS)
                return true;
            else if (result == OpenSmileResult.SMILE_NOT_WRITTEN)
                return false;
            else
            {
                CheckSmileResult(result);
                throw new Exception(); // unreachable as CheckSmileResult will throw, just to make compiler happy
            }
        }

        /// <summary>
        /// Signals the end of the input for the specified cExternalSource component instance.
        /// Attempts to write more data to the component after calling this method will fail.
        /// </summary>
        /// <param name="componentName">Instance name of the cExternalSource component for which to signal the EOI state.</param>
        /// <exception cref="ObjectDisposedException">
        /// Thrown if the object has already been disposed.
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown if openSMILE has not been initialized yet.
        /// </exception>
        /// <exception cref="OpenSmileException">
        /// Thrown for internal openSMILE errors.
        /// </exception>
        public void ExternalSourceSetEoi(string componentName)
        {
            if (disposed)
                throw new ObjectDisposedException(typeof(OpenSMILE).FullName);
            if (smileObj == IntPtr.Zero)
                throw new InvalidOperationException("openSMILE must be initialized first.");

            CheckSmileResult(OpenSmileApi.Smile_extsource_set_external_eoi(smileObj, componentName));
        }

        /// <summary>
        /// Writes an audio buffer to the specified instance of a cExternalAudioSource component.
        /// The data must match the specified data format for the component (sample size, number of channels, etc.).
        /// </summary>
        /// <param name="componentName">Instance name of the cExternalAudioSource component to write to.</param>
        /// <param name="data">Audio frames to write.</param>
        /// <returns>
        /// Returns true if the data was written successfully, otherwise returns false
        /// (e.g. if the internal buffer of the component is full).
        /// </returns>
        /// <exception cref="ObjectDisposedException">
        /// Thrown if the object has already been disposed.
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown if openSMILE has not been initialized yet.
        /// </exception>
        /// <exception cref="OpenSmileException">
        /// Thrown for internal openSMILE errors.
        /// </exception>
        public bool ExternalAudioSourceWriteData(string componentName, byte[] data)
        {
            if (disposed)
                throw new ObjectDisposedException(typeof(OpenSMILE).FullName);
            if (smileObj == IntPtr.Zero)
                throw new InvalidOperationException("openSMILE must be initialized first.");

            OpenSmileResult result = OpenSmileApi.Smile_extaudiosource_write_data(smileObj, componentName, data, data.Length * sizeof(byte));

            if (result == OpenSmileResult.SMILE_SUCCESS)
                return true;
            else if (result == OpenSmileResult.SMILE_NOT_WRITTEN)
                return false;
            else
            {
                CheckSmileResult(result);
                throw new Exception(); // unreachable as CheckSmileResult will throw, just to make compiler happy
            }
        }

        /// <summary>
        /// Writes an audio buffer to the specified instance of a cExternalAudioSource component.
        /// The data must match the specified data format for the component (sample size, number of channels, etc.).
        /// </summary>
        /// <param name="componentName">Instance name of the cExternalAudioSource component to write to.</param>
        /// <param name="data">Audio frames to write.</param>
        /// <returns>
        /// Returns true if the data was written successfully, otherwise returns false
        /// (e.g. if the internal buffer of the component is full).
        /// </returns>
        /// <exception cref="ObjectDisposedException">
        /// Thrown if the object has already been disposed.
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown if openSMILE has not been initialized yet.
        /// </exception>
        /// <exception cref="OpenSmileException">
        /// Thrown for internal openSMILE errors.
        /// </exception>
        public bool ExternalAudioSourceWriteData(string componentName, short[] data)
        {
            if (disposed)
                throw new ObjectDisposedException(typeof(OpenSMILE).FullName);
            if (smileObj == IntPtr.Zero)
                throw new InvalidOperationException("openSMILE must be initialized first.");

            OpenSmileResult result = OpenSmileApi.Smile_extaudiosource_write_data(smileObj, componentName, data, data.Length * sizeof(short));

            if (result == OpenSmileResult.SMILE_SUCCESS)
                return true;
            else if (result == OpenSmileResult.SMILE_NOT_WRITTEN)
                return false;
            else
            {
                CheckSmileResult(result);
                throw new Exception(); // unreachable as CheckSmileResult will throw, just to make compiler happy
            }
        }

        /// <summary>
        /// Writes an audio buffer to the specified instance of a cExternalAudioSource component.
        /// The data must match the specified data format for the component (sample size, number of channels, etc.).
        /// </summary>
        /// <param name="componentName">Instance name of the cExternalAudioSource component to write to.</param>
        /// <param name="data">Audio frames to write.</param>
        /// <returns>
        /// Returns true if the data was written successfully, otherwise returns false
        /// (e.g. if the internal buffer of the component is full).
        /// </returns>
        /// <exception cref="ObjectDisposedException">
        /// Thrown if the object has already been disposed.
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown if openSMILE has not been initialized yet.
        /// </exception>
        /// <exception cref="OpenSmileException">
        /// Thrown for internal openSMILE errors.
        /// </exception>
        public bool ExternalAudioSourceWriteData(string componentName, float[] data)
        {
            if (disposed)
                throw new ObjectDisposedException(typeof(OpenSMILE).FullName);
            if (smileObj == IntPtr.Zero)
                throw new InvalidOperationException("openSMILE must be initialized first.");

            OpenSmileResult result = OpenSmileApi.Smile_extaudiosource_write_data(smileObj, componentName, data, data.Length * sizeof(float));

            if (result == OpenSmileResult.SMILE_SUCCESS)
                return true;
            else if (result == OpenSmileResult.SMILE_NOT_WRITTEN)
                return false;
            else
            {
                CheckSmileResult(result);
                throw new Exception(); // unreachable as CheckSmileResult will throw, just to make compiler happy
            }
        }

        /// <summary>
        /// Writes an audio buffer to the specified instance of a cExternalAudioSource component.
        /// The data must match the specified data format for the component (sample size, number of channels, etc.).
        /// </summary>
        /// <param name="componentName">Instance name of the cExternalAudioSource component to write to.</param>
        /// <param name="data">Audio frames to write.</param>
        /// <returns>
        /// Returns true if the data was written successfully, otherwise returns false
        /// (e.g. if the internal buffer of the component is full).
        /// </returns>
        /// <exception cref="ObjectDisposedException">
        /// Thrown if the object has already been disposed.
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown if openSMILE has not been initialized yet.
        /// </exception>
        /// <exception cref="OpenSmileException">
        /// Thrown for internal openSMILE errors.
        /// </exception>
        public bool ExternalAudioSourceWriteData(string componentName, double[] data)
        {
            if (disposed)
                throw new ObjectDisposedException(typeof(OpenSMILE).FullName);
            if (smileObj == IntPtr.Zero)
                throw new InvalidOperationException("openSMILE must be initialized first.");

            OpenSmileResult result = OpenSmileApi.Smile_extaudiosource_write_data(smileObj, componentName, data, data.Length * sizeof(double));

            if (result == OpenSmileResult.SMILE_SUCCESS)
                return true;
            else if (result == OpenSmileResult.SMILE_NOT_WRITTEN)
                return false;
            else
            {
                CheckSmileResult(result);
                throw new Exception(); // unreachable as CheckSmileResult will throw, just to make compiler happy
            }
        }

        /// <summary>
        /// Signals the end of the input for the specified cExternalAudioSource component instance.
        /// Attempts to write more data to the component after calling this method will fail.
        /// </summary>
        /// <param name="componentName">Instance name of the cExternalAudioSource component for which to signal the EOI state.</param>
        /// <exception cref="ObjectDisposedException">
        /// Thrown if the object has already been disposed.
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown if openSMILE has not been initialized yet.
        /// </exception>
        /// <exception cref="OpenSmileException">
        /// Thrown for internal openSMILE errors.
        /// </exception>
        public void ExternalAudioSourceSetEoi(string componentName)
        {
            if (disposed)
                throw new ObjectDisposedException(typeof(OpenSMILE).FullName);
            if (smileObj == IntPtr.Zero)
                throw new InvalidOperationException("openSMILE must be initialized first.");

            CheckSmileResult(OpenSmileApi.Smile_extaudiosource_set_external_eoi(smileObj, componentName));
        }

        /// <summary>
        /// Sets the callback function for the specified cExternalSink component instance.
        /// The function will get called whenever another openSMILE component writes data to
        /// the cExternalSink component.
        /// </summary>
        /// <param name="componentName">Instance name of the cExternalSink component for which to set the callback.</param>
        /// <exception cref="ObjectDisposedException">
        /// Thrown if the object has already been disposed.
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown if openSMILE has not been initialized yet.
        /// </exception>
        /// <exception cref="OpenSmileException">
        /// Thrown for internal openSMILE errors.
        /// </exception>
        public void ExternalSinkSetCallback(string componentName, ExternalSinkCallback callback)
        {
            if (disposed)
                throw new ObjectDisposedException(typeof(OpenSMILE).FullName);
            if (smileObj == IntPtr.Zero)
                throw new InvalidOperationException("openSMILE must be initialized first.");

            CheckSmileResult(OpenSmileApi.Smile_extsink_set_data_callback(smileObj, componentName, callback, IntPtr.Zero));
        }

        /// <summary>
        /// Sets the callback function for the specified cExternalMessageInterface component instance.
        /// The function will get called whenever the component receives a message.
        /// </summary>
        /// <param name="componentName">Instance name of the cExternalMessageInterface component for which to set the callback.</param>
        /// <exception cref="ObjectDisposedException">
        /// Thrown if the object has already been disposed.
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown if openSMILE has not been initialized yet.
        /// </exception>
        /// <exception cref="OpenSmileException">
        /// Thrown for internal openSMILE errors.
        /// </exception>
        public void ExternalMessageInterfaceSetCallback(string componentName, ExternalMessageInterfaceCallback callback)
        {
            if (disposed)
                throw new ObjectDisposedException(typeof(OpenSMILE).FullName);
            if (smileObj == IntPtr.Zero)
                throw new InvalidOperationException("openSMILE must be initialized first.");

            CheckSmileResult(OpenSmileApi.Smile_extmsginterface_set_msg_callback(smileObj, componentName, callback, IntPtr.Zero));
        }

        /// <summary>
        /// Sets the callback function for the specified cExternalMessageInterface component instance.
        /// The function will get called whenever the component receives a message.
        /// </summary>
        /// <param name="componentName">Instance name of the cExternalMessageInterface component for which to set the callback.</param>
        /// <exception cref="ObjectDisposedException">
        /// Thrown if the object has already been disposed.
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown if openSMILE has not been initialized yet.
        /// </exception>
        /// <exception cref="OpenSmileException">
        /// Thrown for internal openSMILE errors.
        /// </exception>
        public void ExternalMessageInterfaceSetJsonCallback(string componentName, ExternalMessageInterfaceJsonCallback callback)
        {
            if (disposed)
                throw new ObjectDisposedException(typeof(OpenSMILE).FullName);
            if (smileObj == IntPtr.Zero)
                throw new InvalidOperationException("openSMILE must be initialized first.");

            CheckSmileResult(OpenSmileApi.Smile_extmsginterface_set_json_msg_callback(smileObj, componentName, callback, IntPtr.Zero));
        }

        /// <summary>
        /// Starts processing and blocks until finished.
        /// </summary>
        /// <exception cref="ObjectDisposedException">
        /// Thrown if the object has already been disposed.
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown if openSMILE has not been initialized yet.
        /// </exception>
        /// <exception cref="OpenSmileException">
        /// Thrown for internal openSMILE errors.
        /// </exception>
        public void Run()
        {
            if (disposed)
                throw new ObjectDisposedException(typeof(OpenSMILE).FullName);
            if (smileObj == IntPtr.Zero)
                throw new InvalidOperationException("openSMILE must be initialized first.");

            CheckSmileResult(OpenSmileApi.Smile_run(smileObj));
        }

        /// <summary>
        /// Requests abortion of the current run.
        /// Note that openSMILE does not immediately stop after this function returns.
        /// It might continue to run for a short while until the run method returns.
        /// </summary>
        /// <exception cref="ObjectDisposedException">
        /// Thrown if the object has already been disposed.
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown if openSMILE has not been initialized yet.
        /// </exception>
        /// <exception cref="OpenSmileException">
        /// Thrown for internal openSMILE errors.
        /// </exception>
        public void Abort()
        {
            if (disposed)
                throw new ObjectDisposedException(typeof(OpenSMILE).FullName);
            if (smileObj == IntPtr.Zero)
                throw new InvalidOperationException("openSMILE must be initialized first.");

            CheckSmileResult(OpenSmileApi.Smile_abort(smileObj));
        }

        /// <summary>
        /// Resets the internal state of openSMILE after a run has finished or was aborted.
        /// After resetting, you may call <see cref="Run"/> again without the need to call <see cref="Initialize"/> first.
        /// You must re-register any cExternalSink/cExternalMessageInterface callbacks, though.
        /// </summary>
        /// <exception cref="ObjectDisposedException">
        /// Thrown if the object has already been disposed.
        /// </exception>
        /// <exception cref="InvalidOperationException">
        /// Thrown if openSMILE has not been initialized yet.
        /// </exception>
        /// <exception cref="OpenSmileException">
        /// Thrown for internal openSMILE errors.
        /// </exception>
        public void Reset()
        {
            if (disposed)
                throw new ObjectDisposedException(typeof(OpenSMILE).FullName);
            if (smileObj == IntPtr.Zero)
                throw new InvalidOperationException("openSMILE must be initialized first.");

            CheckSmileResult(OpenSmileApi.Smile_reset(smileObj));
        }

        private void CheckSmileResult(OpenSmileResult result)
        {
            if (result != OpenSmileResult.SMILE_SUCCESS)
            {
                string message = OpenSmileApi.Smile_error_msg(smileObj);
                throw new OpenSmileException(result, message);
            }            
        }

        /// <summary>
        /// Frees any internal resources allocated by openSMILE.
        /// </summary>
        public void Dispose()
        {
            Dispose(true);
        }

        protected virtual void Dispose(bool disposing)
        {
            if (disposed)
                return;

            if (disposing)
            {
                // Free any other managed objects here later if structure changed.
            }

            OpenSmileApi.Smile_free(smileObj);
            smileObj = IntPtr.Zero;
            disposed = true;
        }
    }

    [Serializable]
    public class OpenSmileException : Exception
    {
        public OpenSmileResult Code { get; }

        public OpenSmileException(OpenSmileResult code)
        {
            Code = code;
        }

        public OpenSmileException(OpenSmileResult code, string message)
            : base(message)
        {
            Code = code;
        }

        public override string ToString()
        {
            if (string.IsNullOrEmpty(Message))
                return $"Code: {Code}";
            else
                return $"Code: {Code}, Message: {Message}";
        }
    }
}