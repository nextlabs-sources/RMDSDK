using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;

namespace CustomControls.common.sharedResource
{
    internal static class SharedDictionaryManager
    {
        private static ResourceDictionary _stringResource;
        internal static ResourceDictionary StringResource
        {
            get
            {
                if (_stringResource == null)
                {
                    System.Uri resourceLocater = new System.Uri("/CustomControls;component/resources/languages/StringResource.xaml", System.UriKind.Relative);
                    _stringResource = (ResourceDictionary)Application.LoadComponent(resourceLocater);
                }
                return _stringResource;
            }
        }

        private static ResourceDictionary _tabItemStyle;
        internal static ResourceDictionary TabItemStyle
        {
            get
            {
                if (_tabItemStyle == null)
                {
                    System.Uri resourceLocater = new System.Uri("/CustomControls;component/resources/style/TabItemStyle.xaml", System.UriKind.Relative);
                    _tabItemStyle = (ResourceDictionary)Application.LoadComponent(resourceLocater);
                }
                return _tabItemStyle;
            }
        }

        private static ResourceDictionary _unifiedBtnStyle;
        internal static ResourceDictionary UnifiedBtnStyle
        {
            get
            {
                if (_unifiedBtnStyle == null)
                {
                    System.Uri resourceLocater = new System.Uri("/CustomControls;component/resources/style/UnifiedButtonStyle.xaml", System.UriKind.Relative);
                    _unifiedBtnStyle = (ResourceDictionary)Application.LoadComponent(resourceLocater);
                }
                return _unifiedBtnStyle;
            }
        }

        private static ResourceDictionary _unifiedCheckBoxStyle;
        internal static ResourceDictionary UnifiedCheckBoxStyle
        {
            get
            {
                if (_unifiedCheckBoxStyle == null)
                {
                    System.Uri resourceLocater = new System.Uri("/CustomControls;component/resources/style/UnifiedCheckBoxStyle.xaml", System.UriKind.Relative);
                    _unifiedCheckBoxStyle = (ResourceDictionary)Application.LoadComponent(resourceLocater);
                }
                return _unifiedCheckBoxStyle;
            }
        }

    }
}
