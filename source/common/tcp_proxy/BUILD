load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

licenses(["notice"])  # Apache 2

envoy_package()

envoy_cc_library(
    name = "upstream_lib",
    srcs = [
        "upstream.cc",
    ],
    hdrs = [
        "upstream.h",
    ],
    deps = [
        "//envoy/http:header_map_interface",
        "//envoy/router:router_ratelimit_interface",
        "//envoy/tcp:conn_pool_interface",
        "//envoy/tcp:upstream_interface",
        "//envoy/upstream:cluster_manager_interface",
        "//envoy/upstream:load_balancer_interface",
        "//source/common/http:async_client_lib",
        "//source/common/http:codec_client_lib",
        "//source/common/http:hash_policy_lib",
        "//source/common/http:header_map_lib",
        "//source/common/http:headers_lib",
        "//source/common/http:utility_lib",
        "//source/common/network:utility_lib",
        "//source/common/router:header_parser_lib",
        "//source/common/router:router_lib",
        "//source/common/router:shadow_writer_lib",
    ],
)

envoy_cc_library(
    name = "tcp_proxy",
    srcs = [
        "tcp_proxy.cc",
    ],
    hdrs = [
        "tcp_proxy.h",
    ],
    deps = [
        ":upstream_lib",
        "//envoy/access_log:access_log_interface",
        "//envoy/buffer:buffer_interface",
        "//envoy/common:time_interface",
        "//envoy/event:dispatcher_interface",
        "//envoy/network:connection_interface",
        "//envoy/network:filter_interface",
        "//envoy/registry",
        "//envoy/router:router_interface",
        "//envoy/server:filter_config_interface",
        "//envoy/stats:stats_interface",
        "//envoy/stats:stats_macros",
        "//envoy/stats:timespan_interface",
        "//envoy/stream_info:bool_accessor_interface",
        "//envoy/stream_info:filter_state_interface",
        "//envoy/tcp:conn_pool_interface",
        "//envoy/tcp:upstream_interface",
        "//envoy/upstream:cluster_manager_interface",
        "//envoy/upstream:upstream_interface",
        "//source/common/access_log:access_log_lib",
        "//source/common/common:assert_lib",
        "//source/common/common:empty_string",
        "//source/common/common:enum_to_int",
        "//source/common/common:macros",
        "//source/common/common:minimal_logger_lib",
        "//source/common/config:well_known_names",
        "//source/common/formatter:substitution_format_string_lib",
        "//source/common/http:codec_client_lib",
        "//source/common/network:application_protocol_lib",
        "//source/common/network:cidr_range_lib",
        "//source/common/network:filter_lib",
        "//source/common/network:hash_policy_lib",
        "//source/common/network:proxy_protocol_filter_state_lib",
        "//source/common/network:socket_option_factory_lib",
        "//source/common/network:transport_socket_options_lib",
        "//source/common/network:upstream_server_name_lib",
        "//source/common/network:upstream_socket_options_filter_state_lib",
        "//source/common/network:utility_lib",
        "//source/common/protobuf:utility_lib",
        "//source/common/router:metadatamatchcriteria_lib",
        "//source/common/stream_info:stream_id_provider_lib",
        "//source/common/stream_info:stream_info_lib",
        "//source/common/stream_info:uint64_accessor_lib",
        "//source/common/upstream:load_balancer_context_base_lib",
        "//source/common/upstream:od_cds_api_lib",
        "//source/extensions/upstreams/tcp/generic:config",
        "@envoy_api//envoy/config/accesslog/v3:pkg_cc_proto",
        "@envoy_api//envoy/extensions/filters/network/tcp_proxy/v3:pkg_cc_proto",
    ],
)
