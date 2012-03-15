using System;
using Oracle.DataAccess.Client;

namespace SimpleRIS.Properties {
    internal sealed partial class Settings
    {
        private void testConnection(string userId)
        {
            using (OracleConnection conn = new OracleConnection(this.DBConnection))
            {
                conn.Open();
                using (OracleCommand cmdTestUserId = new OracleCommand("select user from dual", conn))
                {
                    string dbUser = cmdTestUserId.ExecuteScalar().ToString();
                    if (!userId.Equals(dbUser, StringComparison.OrdinalIgnoreCase))
                        throw new Exception("更新配置文件失败");
                }
                using (OracleCommand cmdTestDicomSchema = new OracleCommand("alter session set current_schema=dicom", conn))
                {
                    cmdTestDicomSchema.ExecuteNonQuery();
                }
            }
        }

        public void modifyUserAndPassword(string userId, string password)
        {
            if (String.IsNullOrEmpty(userId))
            {
                throw new Exception("请输入用户名");
            }
            else
            {
                OracleConnectionStringBuilder connBuilder = new OracleConnectionStringBuilder { ConnectionString = this["DBConnection"].ToString(), UserID = userId, Password = password };
                this["DBConnection"] = connBuilder.ConnectionString;
                testConnection(userId);
            }
        }
    }
}
