using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServiceManager.rmservmgr.common.helper
{
    /// <summary>
    /// This is used to transform Sync method to Async method gracefully.
    /// </summary>
    public class AsyncHelper
    {
        /// <summary>
        /// Transform the sync operation to async operation
        /// </summary>
        /// <param name="action">the async task execute body</param>
        public static async void RunAsync(Action action)
        {
            await Task.Run(() => { action(); });
        }

        /// <summary>
        /// Transform the sync operation to async operation
        /// </summary>
        /// <param name="action">the async task execute body</param>
        /// <param name="callback">the callback of async task complete</param>
        public static async void RunAsync(Action action, Action callback)
        {
            await Task.Run(() => { action(); });
            callback?.Invoke();
        }

        /// <summary>
        /// Transform the sync operation(with input para) to async operation
        /// </summary>
        /// <typeparam name="T">the input para type of async task</typeparam>
        /// <param name="action">the async task execute body</param>
        /// <param name="para">the input para of async task</param>
        /// <param name="callback">the callback of async task complete</param>
        public static async void RunAsync<T>(Action<T> action, T para, Action callback)
        {
            await Task.Run(() => { action(para); });
            callback?.Invoke();
        }

        /// <summary>
        /// Transform the sync operation(with return value) to async operation
        /// </summary>
        /// <typeparam name="TResult">the returned value type of async task</typeparam>
        /// <param name="function">the async task execute body</param>
        /// <param name="callback">the callback of async task complete</param>
        public static async void RunAsync<TResult>(Func<TResult> function, Action<TResult> callback)
        {
            TResult result = await Task.Run(() => { return function(); });
            callback?.Invoke(result);
        }

        /// <summary>
        /// Transform the sync operation(with input para & return value) to async operation
        /// </summary>
        /// <typeparam name="T">the input para type of async task</typeparam>
        /// <typeparam name="TResult">the returned value type of async task</typeparam>
        /// <param name="function">the async task execute body</param>
        /// <param name="para">the input para of async task</param>
        /// <param name="callback">the callback of async task complete</param>
        public static async void RunAsync<T, TResult>(Func<T, TResult> function, T para, Action<TResult> callback)
        {
            TResult result = await Task.Run(() => { return function(para); });
            callback?.Invoke(result);
        }
    }
}
