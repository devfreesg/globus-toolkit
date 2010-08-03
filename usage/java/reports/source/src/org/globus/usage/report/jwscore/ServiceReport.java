/*
 * Copyright 1999-2006 University of Chicago
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package org.globus.usage.report.jwscore;

import java.sql.ResultSet;

import java.util.HashMap;
import java.util.TreeMap;
import java.util.Comparator;
import java.util.Map;
import java.util.Iterator;
import java.util.StringTokenizer;
import java.util.Date;
import java.util.Calendar;
import java.text.SimpleDateFormat;

import org.globus.usage.report.common.DatabaseRetriever;

public class ServiceReport {

    private Map services = new TreeMap(new StringComparator());
    private Map ipLookupTable = new HashMap();

    private void discoverDomains() {
        Iterator iter = services.entrySet().iterator();
        while(iter.hasNext()) {
            Map.Entry entry = (Map.Entry)iter.next();

            ServiceEntry serviceEntry = (ServiceEntry)entry.getValue();
            serviceEntry.discoverDomains(ipLookupTable);
        }
    }

    private void compute(String listOfServices,
                         int containerType,
                         String ip) {
        // handle the case where all the service names do not fit in the
        // packet
        if (listOfServices.length() >= 1445) {
            int pos = listOfServices.lastIndexOf(',');
            if (pos != -1) {
                listOfServices = listOfServices.substring(0, pos);
            }
        } 

        boolean isPrivateAddress = ServiceEntry.isPrivateAddress(ip);
        if (ip.startsWith("/")) {
            ip = ip.substring(1);
        }
        
        StringTokenizer tokens = new StringTokenizer(listOfServices, ",");
        while(tokens.hasMoreTokens()) {
            String serviceName = tokens.nextToken();
            
            ServiceEntry entry = (ServiceEntry)services.get(serviceName);
            if (entry == null) {
                entry = new ServiceEntry();
                services.put(serviceName, entry);
            }

            switch (containerType) {
            case 1: 
                entry.standalone(); break;
            case 2: 
                entry.servlet(); break;
            default: 
                entry.other(); break;
            }

            if (!isPrivateAddress) {
                entry.addAddress(ip);
            }
        }
    }

    private static class StringComparator implements Comparator {
        public int compare(Object o1, Object o2) {
            String s1 = (String)o1;
            String s2 = (String)o2;
            return s1.compareTo(s2);
        }
    }

    public static void main(String[] args) throws Exception {
            
        String USAGE = "Usage: java ServiceReport [options] <date (yyyy-MM-dd)>";
        String HELP = 
            "Where [options] are:\n" +
            "  -help                    Displays this message\n" +
            "  -step <day|month>        Specifies step type (default: 'day')\n" +
            "  -n <steps>               Specifies number of steps to do to\n" +
            "                           determine end date (default: 1)\n" +
            "  -table TABLENAME         Use TABLENAME instead of the\n" +
            "                           java_ws_core_packets table\n" +
            "\n";
        
        if (args.length == 0) {
            System.err.println(USAGE);
            System.exit(1);
        } else if (args.length == 1 && args[0].equalsIgnoreCase("-help")) {
            System.err.println(USAGE);
            System.err.println(HELP);
            System.exit(1);
        }

        String baseQueryStart = "select service_list,container_type,ip_address from ";
        String table = "java_ws_core_packets";
        String baseQueryEnd = " where event_type = 1 and ";
        int n = 1;
        String containerType = "all";
        String stepStr = "day";

        for (int i=0;i<args.length-1;i++) {
            if (args[i].equals("-n")) {
                n = Integer.parseInt(args[++i]);
            } else if (args[i].equals("-type")) {
                baseQueryEnd += " container_type = " + args[++i] + " and ";
            } else if (args[i].equals("-step")) {
                stepStr = args[++i];
            } else if (args[i].equalsIgnoreCase("-help")) {
                System.err.println(USAGE);
                System.err.println(HELP);
                System.exit(1);
            } else if (args[i].equals("-table")) {
                table = args[++i];
            } else {
                System.err.println("Unknown argument: " + args[i]);
                System.exit(1);
            }
        }

        String inputDate = args[args.length-1];

        // parse step info
        int step = -1;
        if (stepStr.equalsIgnoreCase("day")) {
            step = Calendar.DATE;
        } else if (stepStr.equalsIgnoreCase("month")) {
            step = Calendar.MONTH;
        } else {
            System.err.println("Unsupported step: " + stepStr);
            System.exit(2);
        }

        SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd");

        ServiceReport r = new ServiceReport();

        DatabaseRetriever db = null;

        try {
            db = new DatabaseRetriever();

            Date date = dateFormat.parse(inputDate);
            Calendar calendar = dateFormat.getCalendar();

            if (n < 0) {
                calendar.add(step, n);
                n = -n;
            }

            Date startDate = calendar.getTime();
            calendar.add(step, n);
            Date endDate = calendar.getTime();

            String startDateStr = dateFormat.format(startDate);
            String endDateStr = dateFormat.format(endDate);
            String timeFilter = "DATE(send_time) >= '" + startDateStr + 
                "' and DATE(send_time) < '" + endDateStr + "'";
                

            System.out.println("<service-report container_type=\"" + 
                               containerType + "\">");
            System.out.println("  <start-date>" + startDateStr + "</start-date>");
            System.out.println("  <end-date>" + endDateStr + "</end-date>");


            String query = baseQueryStart + table + baseQueryEnd + timeFilter;

            ResultSet rs = db.retrieve(query);

            while (rs.next()) {
                r.compute(rs.getString(1), rs.getInt(2), rs.getString(3));
            }

            rs.close();

        } finally {
            if (db != null) {
                db.close();
            }
        }

        // generate xml report

        r.discoverDomains();

        System.out.println("  <unique-services>" + r.services.size() + "</unique-services>");
        
        Iterator iter = r.services.entrySet().iterator();
        while(iter.hasNext()) {
            Map.Entry entry = (Map.Entry)iter.next();
            
            ServiceEntry serviceEntry = (ServiceEntry)entry.getValue();
            
            System.out.println("  <entry>");
            
            System.out.println("\t<service-name>" + entry.getKey() + "</service-name>");
            System.out.println("\t<standalone-count>" + serviceEntry.getStandaloneCount() + "</standalone-count>");
            System.out.println("\t<servlet-count>" + serviceEntry.getServletCount() + "</servlet-count>");
            System.out.println("\t<other-count>" + serviceEntry.getOtherCount() + "</other-count>");

            serviceEntry.output(System.out, "\t");
            
            System.out.println("  </entry>");
        }
        
        System.out.println("</service-report>");
    }
    
}