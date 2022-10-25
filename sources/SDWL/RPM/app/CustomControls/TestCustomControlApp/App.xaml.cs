using System;
using System.Collections.Generic;
using System.Configuration;
using System.Data;
using System.Linq;
using System.Threading.Tasks;
using System.Windows;

namespace TestCustomControlApp
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        private void Application_Startup(object sender, StartupEventArgs e)
        {
            List<ResourceDictionary> dictionaryList = new List<ResourceDictionary>();
            foreach (ResourceDictionary dictionary in Application.Current.Resources.MergedDictionaries)
            {
                dictionaryList.Add(dictionary);
            }
            string requestedCulture = string.Format(@"Resources\StringResource.{0}.xaml", "");
            ResourceDictionary resourceDictionary = dictionaryList.FirstOrDefault(d => d.Source.OriginalString.Equals(requestedCulture));
            if (resourceDictionary == null)
            {
                requestedCulture = @"Resources\StringResource.xaml";
                resourceDictionary = dictionaryList.FirstOrDefault(d => d.Source.OriginalString.Equals(requestedCulture));
            }
            if (resourceDictionary != null)
            {
                Application.Current.Resources.MergedDictionaries.Remove(resourceDictionary);
                Application.Current.Resources.MergedDictionaries.Add(resourceDictionary);
            }
        }
    }
}
