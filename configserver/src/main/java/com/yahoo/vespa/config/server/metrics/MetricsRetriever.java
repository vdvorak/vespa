// Copyright 2019 Oath Inc. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
package com.yahoo.vespa.config.server.metrics;

import com.yahoo.slime.ArrayTraverser;
import com.yahoo.slime.Inspector;
import com.yahoo.slime.Slime;
import com.yahoo.vespa.config.SlimeUtils;
import org.apache.http.HttpResponse;
import org.apache.http.client.HttpClient;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.HttpClientBuilder;

import java.io.IOException;
import java.io.InputStream;
import java.io.UncheckedIOException;
import java.net.URI;
import java.time.Instant;


/**
 * Client for reaching out to nodes in an application instance and get their
 * metrics.
 *
 * @author olaa
 * @author ogronnesby
 */
public class MetricsRetriever {
    private final HttpClient httpClient = HttpClientBuilder.create().build();

    /**
     * Call the metrics API on each host in the cluster and aggregate the metrics
     * into a single value.
     */
    public MetricsAggregator requestMetricsForCluster(ClusterInfo clusterInfo) {
        var aggregator = new MetricsAggregator();
        clusterInfo.getHostnames().forEach(host -> getHostMetrics(host, aggregator));
        return aggregator;
    }

    private void getHostMetrics(URI hostURI, MetricsAggregator metrics) {
            Slime responseBody = doMetricsRequest(hostURI);
            Inspector services = responseBody.get().field("services");
            services.traverse((ArrayTraverser) (i, servicesInspector) -> {
                parseService(servicesInspector, metrics);
            });
    }

    private Slime doMetricsRequest(URI hostURI) {
        HttpGet get = new HttpGet(hostURI);
        try {
            HttpResponse response = httpClient.execute(get);
            InputStream is = response.getEntity().getContent();
            Slime slime = SlimeUtils.jsonToSlime(is.readAllBytes());
            is.close();
            return slime;
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    private void parseService(Inspector service, MetricsAggregator metrics) {
        String serviceName = service.field("name").asString();
        Instant timestamp = Instant.ofEpochSecond(service.field("timestamp").asLong());
        metrics.setTimestamp(timestamp);
        service.field("metrics").traverse((ArrayTraverser) (i, m) -> {
            Inspector values = m.field("values");
            switch (serviceName) {
                case "container":
                    metrics.addContainerLatency(
                            values.field("query_latency.sum").asDouble(),
                            values.field("query_latency.count").asDouble());
                    metrics.addFeedLatency(
                            values.field("feed_latency.sum").asDouble(),
                            values.field("feed_latency.count").asDouble());
                    break;
                case "qrserver":
                    metrics.addQrLatency(
                            values.field("query_latency.sum").asDouble(),
                            values.field("query_latency.count").asDouble());
                    break;
                case "distributor":
                    metrics.addDocumentCount(values.field("vds.distributor.docsstored.average").asDouble());
                    break;
            }
        });

    }

}